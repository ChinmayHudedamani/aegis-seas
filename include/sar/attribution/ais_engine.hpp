#pragma once

#include "sar/geo/geo_types.hpp"
#include "sar/attribution/cfar_detector.hpp"
#include <cstdint>
#include <vector>
#include <string>

namespace sar::attribution {

struct AISTransmission {
    std::string mmsi;
    std::string vessel_name;
    std::string vessel_type; // "CARGO", "TANKER", "FISHING", "PASSENGER"
    double lon = 0.0;
    double lat = 0.0;
    float speed_knots = 0.0f;
    float course_deg = 0.0f;
    int64_t timestamp_epoch_sec = 0;
};

struct AISAttributionReport {
    std::string candidate_mmsi;
    std::string vessel_name;
    std::string vessel_type;
    double min_distance_to_origin_km = 0.0;
    float attribution_confidence = 0.0f; // [0.0, 1.0]

    // Red-Team Anomaly & Fraud Flags
    bool is_dark_vessel = false;         // Transponder silence gap > 4h near spill
    double dark_gap_hours = 0.0;
    bool is_spoofed_teleportation = false; // Speed anomaly > 40 kts for cargo
    bool is_high_density_chokepoint = false; // > 15 vessels in search corridor
    size_t co_located_vessel_count = 0;
    float confidence_penalty = 0.0f;

    bool matched_radar_hard_target = false;
    size_t radar_target_id = 0;
    std::string status_summary;
};

class AISEngine {
public:
    explicit AISEngine(double corridor_radius_km = 15.0, 
                       int64_t max_acceptable_gap_sec = 14400); // 4 hours

    /**
     * @brief Correlates AIS vessel transmissions against a reconstructed spill origin and CFAR radar targets.
     */
    std::vector<AISAttributionReport> correlate_vessels(
        const geo::GeoPoint& spill_origin,
        int64_t spill_timestamp_epoch_sec,
        const std::vector<AISTransmission>& ais_feed,
        const std::vector<RadarHardTarget>& radar_targets = {}) const;

private:
    double distance_km(const geo::GeoPoint& p1, const geo::GeoPoint& p2) const;

    double corridor_radius_km_;
    int64_t max_acceptable_gap_sec_;
};

} // namespace sar::attribution
