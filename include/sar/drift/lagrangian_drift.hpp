#pragma once

#include "sar/core/matrix2d.hpp"
#include "sar/geo/geo_types.hpp"
#include <vector>
#include <random>

namespace sar::drift {

struct MetoceanField {
    float surface_current_u_mps = 0.0f; // Eastward current velocity
    float surface_current_v_mps = 0.0f; // Northward current velocity
    float wind_10m_u_mps = 0.0f;        // Eastward wind velocity
    float wind_10m_v_mps = 0.0f;        // Northward wind velocity
    float wind_drift_factor = 0.03f;    // Fay's 3% empirical windage
    float diffusion_coeff_m2ps = 2.5f;  // Turbulent horizontal diffusion (m^2/s)
};

struct Particle {
    double lon;
    double lat;
    double age_hours;
};

struct ReverseDriftResult {
    geo::GeoPoint estimated_origin;
    std::vector<geo::GeoPoint> confidence_polygon_95; // 95% spatial search ellipse
    double corridor_area_km2 = 0.0;
    bool excessive_uncertainty_warning = false;       // Triggered if area > max_search_area_km2
    std::string warning_message;
    std::vector<Particle> particle_cloud;
};

class LagrangianDriftEngine {
public:
    explicit LagrangianDriftEngine(size_t num_particles = 1000, 
                                   double max_search_area_km2 = 500.0);

    /**
     * @brief Backward-in-time Monte Carlo Lagrangian particle tracking to reconstruct spill origin.
     * @param observed_centroid Centroid of the slick when observed by SAR satellite.
     * @param backward_hours Time duration in hours to trace backward (e.g. -6.0 hours).
     * @param metocean Metocean current & wind vector fields.
     */
    ReverseDriftResult trace_backward(
        const geo::GeoPoint& observed_centroid,
        double backward_hours,
        const MetoceanField& metocean,
        unsigned int seed = 42) const;

private:
    size_t num_particles_;
    double max_search_area_km2_;
};

} // namespace sar::drift
