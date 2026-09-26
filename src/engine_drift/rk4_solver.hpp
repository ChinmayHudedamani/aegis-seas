#pragma once

#include "sar/geo/geo_types.hpp"
#include <vector>
#include <random>
#include <cmath>
#include <string>

namespace aegis::drift {

struct MetoceanField2D {
    float surface_current_u_mps = 0.22f; // Eastward current velocity
    float surface_current_v_mps = -0.08f; // Northward current velocity
    float wind_10m_u_mps = -5.89f;       // Eastward wind velocity (240 deg SW monsoon)
    float wind_10m_v_mps = -3.40f;       // Northward wind velocity
    float wind_drift_factor = 0.035f;    // Fay's 3.5% windage
    float coriolis_deflection_deg = 12.0f; // Northern hemisphere deflection
    float diffusion_coeff_m2ps = 7.5f;   // Horizontal eddy diffusivity D_h
};

struct Parcel {
    double lon = 0.0;
    double lat = 0.0;
    double age_seconds = 0.0;
};

struct ConfidenceEllipse95 {
    sar::geo::GeoPoint centroid;
    double semi_major_axis_m = 0.0;
    double semi_minor_axis_m = 0.0;
    double orientation_deg = 0.0;
    double area_km2 = 0.0;
    std::vector<sar::geo::GeoPoint> polygon_ring;
};

struct LagrangianDriftReport {
    sar::geo::GeoPoint estimated_origin;
    ConfidenceEllipse95 search_ellipse_95;
    std::vector<Parcel> final_parcels;
    bool warning_excessive_uncertainty = false;
    std::string warning_message;
};

class RK4StochasticDriftSolver {
public:
    explicit RK4StochasticDriftSolver(size_t num_particles = 5000,
                                     double max_search_area_km2 = 500.0);

    /**
     * @brief Execute 4th-Order Runge-Kutta stochastic drift simulation (dt < 0 for Hindcast, dt > 0 for Forecast).
     */
    LagrangianDriftReport solve_drift(
        const std::vector<sar::geo::GeoPoint>& initial_polygon,
        const sar::geo::GeoPoint& observed_centroid,
        double total_duration_seconds,
        double step_size_seconds,
        const MetoceanField2D& metocean,
        unsigned int seed = 42) const;

    /**
     * @brief Calculate hydrodynamics advection vector including Stokes drift & Coriolis matrix.
     */
    static std::pair<double, double> compute_hydrodynamic_velocity(
        double lat, double lon, const MetoceanField2D& metocean);

    /**
     * @brief Analytical 2x2 eigendecomposition to construct 95% spatial confidence search ellipse.
     */
    static ConfidenceEllipse95 compute_95_confidence_ellipse(
        const std::vector<Parcel>& parcels, double center_lat);

private:
    size_t num_particles_;
    double max_search_area_km2_;
};

} // namespace aegis::drift
