#include "sar/drift/lagrangian_drift.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>

using namespace std;

namespace sar::drift {

LagrangianDriftEngine::LagrangianDriftEngine(size_t num_particles, double max_search_area_km2)
    : num_particles_(num_particles),
      max_search_area_km2_(max_search_area_km2) {}

ReverseDriftResult LagrangianDriftEngine::trace_backward(
    const geo::GeoPoint& observed_centroid,
    double backward_hours,
    const MetoceanField& metocean,
    unsigned int seed) const 
{
    ReverseDriftResult res;
    mt19937 rng(seed);

    constexpr double R_earth = 6378137.0;
    constexpr double deg_to_rad = 3.141592653589793 / 180.0;
    constexpr double rad_to_deg = 180.0 / 3.141592653589793;

    double dt_sec = 60.0; // 1-minute numerical integration time step
    double total_sec = abs(backward_hours) * 3600.0;
    size_t num_steps = static_cast<size_t>(total_sec / dt_sec);

    // Deterministic advection velocity vector: V = Current + 3% Wind
    double u_adv = metocean.surface_current_u_mps + metocean.wind_drift_factor * metocean.wind_10m_u_mps;
    double v_adv = metocean.surface_current_v_mps + metocean.wind_drift_factor * metocean.wind_10m_v_mps;

    // Backward advection reverses the velocity vector (-1.0)
    double u_back = -u_adv;
    double v_back = -v_adv;

    // Turbulent random walk dispersion standard deviation per time step: sigma = sqrt(2 * D * dt)
    double random_walk_std = sqrt(2.0 * metocean.diffusion_coeff_m2ps * dt_sec);
    normal_distribution<double> norm_dist(0.0, random_walk_std);

    res.particle_cloud.resize(num_particles_);

    // Initialize all particles at observed centroid
    for (size_t i = 0; i < num_particles_; ++i) {
        res.particle_cloud[i] = {observed_centroid.lon, observed_centroid.lat, 0.0};
    }

    // Velocity uncertainty across ECMWF/CMEMS ensemble members (std = 0.35 m/s)
    normal_distribution<double> u_vel_dist(u_back, 0.35);
    normal_distribution<double> v_vel_dist(v_back, 0.35);

    // Integrate backward in time
    for (size_t i = 0; i < num_particles_; ++i) {
        double p_u = u_vel_dist(rng);
        double p_v = v_vel_dist(rng);

        for (size_t step = 0; step < num_steps; ++step) {
            double dx_m = p_u * dt_sec + norm_dist(rng);
            double dy_m = p_v * dt_sec + norm_dist(rng);

            double lat_rad = res.particle_cloud[i].lat * deg_to_rad;
            double d_lon = (dx_m / (R_earth * cos(lat_rad))) * rad_to_deg;
            double d_lat = (dy_m / R_earth) * rad_to_deg;

            res.particle_cloud[i].lon += d_lon;
            res.particle_cloud[i].lat += d_lat;
        }
        res.particle_cloud[i].age_hours = total_sec / 3600.0;
    }

    // Compute ensemble centroid (estimated spill origin)
    double sum_lon = 0.0, sum_lat = 0.0;
    for (const auto& p : res.particle_cloud) {
        sum_lon += p.lon;
        sum_lat += p.lat;
    }
    res.estimated_origin.lon = sum_lon / num_particles_;
    res.estimated_origin.lat = sum_lat / num_particles_;

    // Calculate spatial covariance matrix of particle cloud (in meters)
    double lat_rad_center = res.estimated_origin.lat * deg_to_rad;
    double var_x = 0.0, var_y = 0.0, cov_xy = 0.0;

    for (const auto& p : res.particle_cloud) {
        double dx = (p.lon - res.estimated_origin.lon) * (deg_to_rad * R_earth * cos(lat_rad_center));
        double dy = (p.lat - res.estimated_origin.lat) * (deg_to_rad * R_earth);
        var_x += dx * dx;
        var_y += dy * dy;
        cov_xy += dx * dy;
    }
    var_x /= num_particles_;
    var_y /= num_particles_;
    cov_xy /= num_particles_;

    // Eigenvalues of covariance matrix for 95% confidence ellipse (chi-squared factor k = 2.4477 for 95%)
    double common = sqrt((var_x - var_y) * (var_x - var_y) + 4.0 * cov_xy * cov_xy);
    double lambda1 = (var_x + var_y + common) / 2.0;
    double lambda2 = max(1.0, (var_x + var_y - common) / 2.0);

    constexpr double k_95 = 2.4477;
    double major_axis_m = k_95 * sqrt(lambda1);
    double minor_axis_m = k_95 * sqrt(lambda2);
    double theta_rad = 0.5 * atan2(2.0 * cov_xy, var_x - var_y);

    // 95% Confidence Search Corridor Area (km^2)
    constexpr double pi = 3.141592653589793;
    res.corridor_area_km2 = (pi * major_axis_m * minor_axis_m) / 1.0e6;

    // Check uncertainty expansion threshold
    if (res.corridor_area_km2 > max_search_area_km2_) {
        res.excessive_uncertainty_warning = true;
        ostringstream oss;
        oss << "WARNING: Reverse-drift 95% search corridor (" << res.corridor_area_km2 
            << " km^2) exceeds operational attribution limit (" << max_search_area_km2_ 
            << " km^2). High risk of multi-vessel attribution ambiguity.";
        res.warning_message = oss.str();
    }

    // Construct 95% confidence boundary polygon
    constexpr int num_vertices = 24;
    for (int i = 0; i < num_vertices; ++i) {
        double alpha = i * (2.0 * pi / num_vertices);
        double ex = major_axis_m * cos(alpha);
        double ey = minor_axis_m * sin(alpha);

        // Rotate by theta
        double rx = ex * cos(theta_rad) - ey * sin(theta_rad);
        double ry = ex * sin(theta_rad) + ey * cos(theta_rad);

        double v_lon = res.estimated_origin.lon + (rx / (R_earth * cos(lat_rad_center))) * rad_to_deg;
        double v_lat = res.estimated_origin.lat + (ry / R_earth) * rad_to_deg;
        res.confidence_polygon_95.push_back({v_lon, v_lat});
    }
    res.confidence_polygon_95.push_back(res.confidence_polygon_95.front());

    return res;
}

} // namespace sar::drift
