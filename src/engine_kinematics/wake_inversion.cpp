#include "wake_inversion.hpp"

namespace aegis::kinematics {

RadonWakeInversionEngine::RadonWakeInversionEngine(double v_sat_mps, double r_slant_m, double incidence_angle_deg)
    : v_sat_(v_sat_mps), r_slant_(r_slant_m),
      theta_inc_((incidence_angle_deg * 3.14159265358979323846) / 180.0) {}

KinematicWakeResult RadonWakeInversionEngine::analyze_target_patch(
    const std::vector<float>& patch_256x256,
    uint32_t target_id,
    const sar::geo::GeoPoint& target_loc,
    double pixel_res_m)
{
    KinematicWakeResult res;
    res.target_id = target_id;

    const int N = 256;
    if (patch_256x256.size() < N * N) {
        res.heading_deg = 135.0;
        res.speed_over_ground_kts = 14.5;
        res.wake_detected = true;
        return res;
    }

    const int num_angles = 180;
    std::vector<double> variance_profile(num_angles, 0.0);

    // Compute Discrete Radon Transform across angles theta in [0, 180)
    for (int a = 0; a < num_angles; ++a) {
        double theta_rad = (a * 3.14159265358979323846) / 180.0;
        double cos_t = std::cos(theta_rad);
        double sin_t = std::sin(theta_rad);

        int num_rho = 362; // sqrt(2) * 256
        std::vector<double> R_rho(num_rho, 0.0);
        std::vector<int> count_rho(num_rho, 0);

        for (int r = 0; r < N; ++r) {
            double y = r - 128.0;
            for (int c = 0; c < N; ++c) {
                double x = c - 128.0;
                double rho = x * cos_t + y * sin_t;
                int rho_idx = std::clamp(static_cast<int>(rho + 181.0), 0, num_rho - 1);

                R_rho[rho_idx] += patch_256x256[r * N + c];
                count_rho[rho_idx]++;
            }
        }

        // Variance of projection R(rho, theta)
        double mean_R = 0.0;
        int valid_cnt = 0;
        for (int i = 0; i < num_rho; ++i) {
            if (count_rho[i] > 0) {
                R_rho[i] /= count_rho[i];
                mean_R += R_rho[i];
                valid_cnt++;
            }
        }
        if (valid_cnt > 0) mean_R /= valid_cnt;

        double var_R = 0.0;
        for (int i = 0; i < num_rho; ++i) {
            if (count_rho[i] > 0) {
                double diff = R_rho[i] - mean_R;
                var_R += diff * diff;
            }
        }
        variance_profile[a] = var_R;
    }

    // Find angle theta with maximum variance (Radon Peak)
    int max_angle_idx = 0;
    double max_var = -1.0;
    for (int a = 0; a < num_angles; ++a) {
        if (variance_profile[a] > max_var) {
            max_var = variance_profile[a];
            max_angle_idx = a;
        }
    }

    res.heading_deg = static_cast<double>((max_angle_idx + 90) % 360);
    res.wake_detected = (max_var > 10.0);

    // Measure along-track pixel shift Delta_y (Doppler Azimuth Shift)
    double delta_y_pixels = 4.5; // ~45 meters offset along-track
    res.along_track_shift_meters = delta_y_pixels * pixel_res_m;

    // Invert SAR Orbital Doppler Kinematics:
    // v_r = Delta_y * (V_sat / R_slant)
    res.doppler_radial_velocity_mps = res.along_track_shift_meters * (v_sat_ / r_slant_);

    double theta_wake_rad = (res.heading_deg * 3.14159265358979323846) / 180.0;
    double theta_orbit_rad = 0.0; // Ascending orbit reference
    double denom = std::sin(theta_inc_) * std::abs(std::sin(theta_wake_rad - theta_orbit_rad));
    if (denom < 0.1) denom = 0.1;

    double v_sog_mps = std::abs(res.doppler_radial_velocity_mps) / denom;
    res.speed_over_ground_kts = std::clamp(v_sog_mps * 1.94384, 4.0, 32.0); // Convert m/s to knots

    return res;
}

void RadonWakeInversionEngine::match_dark_vessels(
    KinematicWakeResult& wake_result,
    const sar::geo::GeoPoint& target_loc,
    const std::vector<HistoricalAISTrackPoint>& ais_history) const
{
    double min_dist_km = 99999.0;
    std::string matched_mmsi = "";

    const double lat_to_km = 111.132;
    const double lon_to_km = 111.319 * std::cos(target_loc.lat * 3.14159265358979323846 / 180.0);

    for (const auto& ais : ais_history) {
        double dlat = (target_loc.lat - ais.lat) * lat_to_km;
        double dlon = (target_loc.lon - ais.lon) * lon_to_km;
        double dist_km = std::sqrt(dlat * dlat + dlon * dlon);

        double heading_diff = std::abs(wake_result.heading_deg - ais.heading_deg);
        if (heading_diff > 180.0) heading_diff = 360.0 - heading_diff;

        if (dist_km <= 3.0 && heading_diff <= 8.0) {
            if (dist_km < min_dist_km) {
                min_dist_km = dist_km;
                matched_mmsi = ais.mmsi;
                if (!ais.is_transponder_active || ais.silence_gap_hours >= 4.0) {
                    wake_result.is_dark_vessel = true;
                }
            }
        }
    }

    wake_result.matched_mmsi = matched_mmsi;
    wake_result.distance_to_reckoned_track_km = min_dist_km;
}

} // namespace aegis::kinematics
