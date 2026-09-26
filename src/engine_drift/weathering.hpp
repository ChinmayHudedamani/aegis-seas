#pragma once

#include <cmath>
#include <string>
#include <algorithm>
#include <iostream>

namespace aegis::drift {

struct WeatheringState {
    double fractal_dimension = 1.10;
    std::string morphological_classification = "Fresh discharge (< 3 hours)";
    double fraction_evaporated = 0.20;
    double mass_transfer_coeff_km = 0.005;
    double elapsed_age_seconds = 14400.0; // Default ~4 hours
    double age_uncertainty_seconds = 1800.0; // +/- 30 mins
};

class MackayWeatheringEngine {
public:
    MackayWeatheringEngine(double sea_temp_K = 298.15,
                           double crude_density_kg_m3 = 860.0,
                           double initial_thickness_m = 1e-4);

    /**
     * @brief Compute fractal dimension D = 2 * ln(P / 4) / ln(A)
     */
    double compute_fractal_dimension(double area_m2, double perimeter_m) const;

    /**
     * @brief Evaluate physical chemical age via inverted Mackay evaporation kinetics.
     */
    WeatheringState invert_spill_age(double area_m2, double perimeter_m,
                                    float mean_damping_db, double wind_speed_10m) const;

private:
    double T_K_;         // Sea Surface Temp (Kelvin)
    double T_G_;         // Distillation slope (380 K)
    double rho_oil_;     // Crude density (860 kg/m^3)
    double d_0_;         // Initial slick thickness (1e-4 m)
    double P_0_;         // Vapor pressure (1.5e4 Pa)
    double M_w_;         // Molar mass (0.22 kg/mol)
    double R_;           // Universal gas constant (8.314 J/mol*K)
};

} // namespace aegis::drift
