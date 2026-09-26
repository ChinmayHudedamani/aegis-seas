#include "weathering.hpp"

namespace aegis::drift {

MackayWeatheringEngine::MackayWeatheringEngine(double sea_temp_K,
                                               double crude_density_kg_m3,
                                               double initial_thickness_m)
    : T_K_(sea_temp_K),
      T_G_(380.0),
      rho_oil_(crude_density_kg_m3),
      d_0_(initial_thickness_m),
      P_0_(1.5e4),
      M_w_(0.22),
      R_(8.314) {}

double MackayWeatheringEngine::compute_fractal_dimension(double area_m2, double perimeter_m) const {
    if (area_m2 <= 1.0 || perimeter_m <= 4.0) return 1.0;
    double numerator = std::log(perimeter_m / 4.0);
    double denominator = std::log(area_m2);
    if (denominator <= 0.0) return 1.0;
    return 2.0 * (numerator / denominator);
}

WeatheringState MackayWeatheringEngine::invert_spill_age(double area_m2, double perimeter_m,
                                                         float mean_damping_db, double wind_speed_10m) const
{
    WeatheringState state;
    state.fractal_dimension = compute_fractal_dimension(area_m2, perimeter_m);

    // Classify Morphological State
    if (state.fractal_dimension < 1.15) {
        state.morphological_classification = "Fresh discharge (< 3 hours)";
    } else if (state.fractal_dimension <= 1.30) {
        state.morphological_classification = "Intermediate weathering (3 - 12 hours)";
    } else {
        state.morphological_classification = "Heavily weathered ribbon/windrow (> 18 hours)";
    }

    // Mass transfer coefficient: K_m = 0.002 * U_10^0.78
    double u10 = std::max(1.0, wind_speed_10m);
    state.mass_transfer_coeff_km = 0.002 * std::pow(u10, 0.78);

    // Fraction evaporated derived from radar damping depth
    double damping = std::abs(static_cast<double>(mean_damping_db));
    state.fraction_evaporated = std::clamp((damping - 3.0) / 8.0, 0.05, 0.55);

    // Analytical Mackay Equation for physical time t_age
    // t_age = [exp((T_G * F_evap) / T_K) - 1] * [(R * T_K * rho_oil * d_0) / (K_m * P_0 * M_w)]
    double term1 = std::exp((T_G_ * state.fraction_evaporated) / T_K_) - 1.0;
    double term2 = (R_ * T_K_ * rho_oil_ * d_0_) / (state.mass_transfer_coeff_km * P_0_ * M_w_);

    state.elapsed_age_seconds = std::max(300.0, term1 * term2);
    state.age_uncertainty_seconds = state.elapsed_age_seconds * 0.15; // 15% uncertainty bound

    return state;
}

} // namespace aegis::drift
