#include "rk4_solver.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace aegis::drift {

RK4StochasticDriftSolver::RK4StochasticDriftSolver(size_t num_particles, double max_search_area_km2)
    : num_particles_(num_particles), max_search_area_km2_(max_search_area_km2) {}

std::pair<double, double> RK4StochasticDriftSolver::compute_hydrodynamic_velocity(
    double lat, double lon, const MetoceanField2D& metocean)
{
    (void)lon; // Silence unused parameter warning

    // 10m Wind Speed vector
    double uw = metocean.wind_10m_u_mps;
    double vw = metocean.wind_10m_v_mps;

    // Coriolis deflection matrix R(theta_d) (12 deg clockwise in Northern Hemisphere)
    double theta_rad = (metocean.coriolis_deflection_deg * 3.14159265358979323846) / 180.0;
    if (lat < 0.0) theta_rad = -theta_rad; // Reverse for Southern Hemisphere

    double cos_t = std::cos(theta_rad);
    double sin_t = std::sin(theta_rad);

    // Rotated Stokes Wind Drift Vector
    double u_stokes_rot = cos_t * uw + sin_t * vw;
    double v_stokes_rot = -sin_t * uw + cos_t * vw;

    double u_stokes = metocean.wind_drift_factor * u_stokes_rot;
    double v_stokes = metocean.wind_drift_factor * v_stokes_rot;

    // Total Hydrodynamic Advection Velocity (Currents + Stokes Drift)
    double u_total = metocean.surface_current_u_mps + u_stokes;
    double v_total = metocean.surface_current_v_mps + v_stokes;

    return {u_total, v_total};
}

LagrangianDriftReport RK4StochasticDriftSolver::solve_drift(
    const std::vector<sar::geo::GeoPoint>& initial_polygon,
    const sar::geo::GeoPoint& observed_centroid,
    double total_duration_seconds,
    double step_size_seconds,
    const MetoceanField2D& metocean,
    unsigned int seed) const
{
    LagrangianDriftReport report;
    std::mt19937 gen(seed);
    std::normal_distribution<double> gaussian(0.0, 1.0);

    // Initialize N_p = 5,000 Monte Carlo parcels
    std::vector<Parcel> parcels(num_particles_);
    const size_t num_verts = initial_polygon.empty() ? 1 : initial_polygon.size();

    for (size_t i = 0; i < num_particles_; ++i) {
        if (!initial_polygon.empty()) {
            const auto& pt = initial_polygon[i % num_verts];
            // Add slight spatial jitter across slick extent
            parcels[i].lon = pt.lon + (gaussian(gen) * 0.0005);
            parcels[i].lat = pt.lat + (gaussian(gen) * 0.0005);
        } else {
            parcels[i].lon = observed_centroid.lon + (gaussian(gen) * 0.0005);
            parcels[i].lat = observed_centroid.lat + (gaussian(gen) * 0.0005);
        }
        parcels[i].age_seconds = 0.0;
    }

    const double dt = step_size_seconds;
    const size_t num_steps = static_cast<size_t>(std::abs(total_duration_seconds / dt));
    const double D_h = metocean.diffusion_coeff_m2ps;
    const double diff_scale = std::sqrt(2.0 * D_h * std::abs(dt));

    const double lat_to_m = 111132.954;

    // Execute 4th-Order Runge-Kutta time steps
    for (size_t step = 0; step < num_steps; ++step) {
        for (auto& p : parcels) {
            double lon_to_m = 111319.488 * std::cos(p.lat * 3.14159265358979323846 / 180.0);

            // k1
            auto [u1, v1] = compute_hydrodynamic_velocity(p.lat, p.lon, metocean);

            // k2
            double lat_k2 = p.lat + (0.5 * dt * v1) / lat_to_m;
            double lon_k2 = p.lon + (0.5 * dt * u1) / lon_to_m;
            auto [u2, v2] = compute_hydrodynamic_velocity(lat_k2, lon_k2, metocean);

            // k3
            double lat_k3 = p.lat + (0.5 * dt * v2) / lat_to_m;
            double lon_k3 = p.lon + (0.5 * dt * u2) / lon_to_m;
            auto [u3, v3] = compute_hydrodynamic_velocity(lat_k3, lon_k3, metocean);

            // k4
            double lat_k4 = p.lat + (dt * v3) / lat_to_m;
            double lon_k4 = p.lon + (dt * u3) / lon_to_m;
            auto [u4, v4] = compute_hydrodynamic_velocity(lat_k4, lon_k4, metocean);

            // Deterministic RK4 Advection
            double dx_det = (dt / 6.0) * (u1 + 2.0 * u2 + 2.0 * u3 + u4);
            double dy_det = (dt / 6.0) * (v1 + 2.0 * v2 + 2.0 * v3 + v4);

            // Stochastic Brownian Dispersion
            double dx_diff = diff_scale * gaussian(gen);
            double dy_diff = diff_scale * gaussian(gen);

            double dx_total = dx_det + dx_diff;
            double dy_total = dy_det + dy_diff;

            p.lat += dy_total / lat_to_m;
            p.lon += dx_total / lon_to_m;
            p.age_seconds += std::abs(dt);
        }
    }

    report.final_parcels = parcels;
    report.search_ellipse_95 = compute_95_confidence_ellipse(parcels, observed_centroid.lat);
    report.estimated_origin = report.search_ellipse_95.centroid;

    if (report.search_ellipse_95.area_km2 > max_search_area_km2_) {
        report.warning_excessive_uncertainty = true;
        report.warning_message = "WARNING: Reverse-drift 95% search corridor (" + 
                                 std::to_string(report.search_ellipse_95.area_km2) + 
                                 " km^2) exceeds operational attribution limit (" + 
                                 std::to_string(max_search_area_km2_) + " km^2). High risk of multi-vessel attribution ambiguity.";
    }

    return report;
}

ConfidenceEllipse95 RK4StochasticDriftSolver::compute_95_confidence_ellipse(
    const std::vector<Parcel>& parcels, double center_lat)
{
    ConfidenceEllipse95 ellipse;
    if (parcels.empty()) return ellipse;

    double sum_lat = 0.0, sum_lon = 0.0;
    for (const auto& p : parcels) {
        sum_lat += p.lat;
        sum_lon += p.lon;
    }
    ellipse.centroid.lat = sum_lat / parcels.size();
    ellipse.centroid.lon = sum_lon / parcels.size();

    const double lat_to_m = 111132.954;
    const double lon_to_m = 111319.488 * std::cos(center_lat * 3.14159265358979323846 / 180.0);

    // Compute 2x2 Spatial Covariance Matrix Sigma
    double s_xx = 0.0, s_yy = 0.0, s_xy = 0.0;
    for (const auto& p : parcels) {
        double dx = (p.lon - ellipse.centroid.lon) * lon_to_m;
        double dy = (p.lat - ellipse.centroid.lat) * lat_to_m;
        s_xx += dx * dx;
        s_yy += dy * dy;
        s_xy += dx * dy;
    }
    double N = static_cast<double>(parcels.size() > 1 ? parcels.size() - 1 : 1);
    s_xx /= N;
    s_yy /= N;
    s_xy /= N;

    // Analytical Eigendecomposition of 2x2 Covariance Matrix
    double trace = s_xx + s_yy;
    double det = s_xx * s_yy - s_xy * s_xy;
    double term = std::sqrt(std::max(0.0, (trace * trace / 4.0) - det));

    double lambda1 = (trace / 2.0) + term;
    double lambda2 = (trace / 2.0) - term;

    // 95% Confidence scale factor (Chi-square for 2 DoF = 5.991)
    ellipse.semi_major_axis_m = std::sqrt(5.991 * std::max(0.0, lambda1));
    ellipse.semi_minor_axis_m = std::sqrt(5.991 * std::max(0.0, lambda2));

    double theta_rad = 0.5 * std::atan2(2.0 * s_xy, s_xx - s_yy);
    ellipse.orientation_deg = (theta_rad * 180.0) / 3.14159265358979323846;
    ellipse.area_km2 = (3.14159265358979323846 * ellipse.semi_major_axis_m * ellipse.semi_minor_axis_m) / 1e6;

    // Generate 32-vertex spatial confidence polygon ring
    const size_t num_points = 32;
    for (size_t i = 0; i < num_points; ++i) {
        double phi = (2.0 * 3.14159265358979323846 * i) / num_points;
        double ex = ellipse.semi_major_axis_m * std::cos(phi);
        double ey = ellipse.semi_minor_axis_m * std::sin(phi);

        // Rotate by orientation theta
        double rx = ex * std::cos(theta_rad) - ey * std::sin(theta_rad);
        double ry = ex * std::sin(theta_rad) + ey * std::cos(theta_rad);

        sar::geo::GeoPoint pt;
        pt.lat = ellipse.centroid.lat + (ry / lat_to_m);
        pt.lon = ellipse.centroid.lon + (rx / lon_to_m);
        ellipse.polygon_ring.push_back(pt);
    }
    if (!ellipse.polygon_ring.empty()) {
        ellipse.polygon_ring.push_back(ellipse.polygon_ring.front());
    }

    return ellipse;
}

} // namespace aegis::drift
