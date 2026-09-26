#pragma once

#include "sar/geo/geo_types.hpp"
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <iostream>
#include <cstdint>

namespace aegis::kinematics {

struct KinematicWakeResult {
    uint32_t target_id = 0;
    double heading_deg = 0.0;
    double speed_over_ground_kts = 0.0;
    double doppler_radial_velocity_mps = 0.0;
    double along_track_shift_meters = 0.0;
    bool wake_detected = false;
    bool is_dark_vessel = false;
    std::string matched_mmsi = "";
    double distance_to_reckoned_track_km = 0.0;
};

struct HistoricalAISTrackPoint {
    std::string mmsi;
    std::string vessel_name;
    std::string vessel_type;
    double lat = 0.0;
    double lon = 0.0;
    double heading_deg = 0.0;
    double speed_kts = 0.0;
    double timestamp_utc = 0.0;
    bool is_transponder_active = true;
    double silence_gap_hours = 0.0;
};

class RadonWakeInversionEngine {
public:
    RadonWakeInversionEngine(double v_sat_mps = 7500.0,
                             double r_slant_m = 850000.0,
                             double incidence_angle_deg = 35.0);

    /**
     * @brief Compute discrete Radon Transform over 256x256 sub-patch to extract wake angle & Doppler velocity.
     */
    KinematicWakeResult analyze_target_patch(
        const std::vector<float>& patch_256x256,
        uint32_t target_id,
        const sar::geo::GeoPoint& target_loc,
        double pixel_res_m = 10.0);

    /**
     * @brief Perform spatial dead-reckoning correlation against historical AIS vessel database to unmask dark vessels.
     */
    void match_dark_vessels(
        KinematicWakeResult& wake_result,
        const sar::geo::GeoPoint& target_loc,
        const std::vector<HistoricalAISTrackPoint>& ais_history) const;

private:
    double v_sat_;       // Satellite orbital speed (~7500 m/s)
    double r_slant_;     // Slant range distance (~850 km)
    double theta_inc_;   // Incidence angle in radians
};

} // namespace aegis::kinematics
