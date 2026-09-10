#include "sar/attribution/ais_engine.hpp"
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <map>
#include <sstream>

using namespace std;

namespace sar::attribution {

AISEngine::AISEngine(double corridor_radius_km, int64_t max_acceptable_gap_sec)
    : corridor_radius_km_(corridor_radius_km),
      max_acceptable_gap_sec_(max_acceptable_gap_sec) {}

double AISEngine::distance_km(const geo::GeoPoint& p1, const geo::GeoPoint& p2) const {
    constexpr double R_earth_km = 6371.0;
    constexpr double deg_to_rad = 3.141592653589793 / 180.0;

    double dlat = (p2.lat - p1.lat) * deg_to_rad;
    double dlon = (p2.lon - p1.lon) * deg_to_rad;

    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(p1.lat * deg_to_rad) * cos(p2.lat * deg_to_rad) *
               sin(dlon / 2.0) * sin(dlon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(max(0.0, 1.0 - a)));
    return R_earth_km * c;
}

vector<AISAttributionReport> AISEngine::correlate_vessels(
    const geo::GeoPoint& spill_origin,
    int64_t spill_timestamp_epoch_sec,
    const vector<AISTransmission>& ais_feed,
    const vector<RadarHardTarget>& radar_targets) const 
{
    // Group AIS messages by MMSI
    map<string, vector<AISTransmission>> tracks_by_mmsi;
    for (const auto& msg : ais_feed) {
        tracks_by_mmsi[msg.mmsi].push_back(msg);
    }

    vector<AISAttributionReport> candidates;

    for (auto& [mmsi, track] : tracks_by_mmsi) {
        // Sort chronologically
        sort(track.begin(), track.end(), [](const AISTransmission& a, const AISTransmission& b) {
            return a.timestamp_epoch_sec < b.timestamp_epoch_sec;
        });

        double min_dist_km = 1e9;
        int64_t closest_time_sec = 0;
        bool speed_spoofed = false;
        bool dark_gap = false;
        double max_gap_hours = 0.0;

        for (size_t i = 0; i < track.size(); ++i) {
            const auto& pt = track[i];
            geo::GeoPoint pos{pt.lon, pt.lat};
            double dist = distance_km(pos, spill_origin);
            if (dist < min_dist_km) {
                min_dist_km = dist;
                closest_time_sec = pt.timestamp_epoch_sec;
            }

            // Anomaly Check: Cargo/Tanker speed > 40 knots (Teleportation/GPS Spoof)
            if ((pt.vessel_type == "CARGO" || pt.vessel_type == "TANKER") && pt.speed_knots > 40.0f) {
                speed_spoofed = true;
            }

            // Anomaly Check: Temporal gaps between consecutive pings
            if (i > 0) {
                int64_t gap_sec = pt.timestamp_epoch_sec - track[i - 1].timestamp_epoch_sec;
                if (gap_sec > max_acceptable_gap_sec_) {
                    dark_gap = true;
                    max_gap_hours = max(max_gap_hours, gap_sec / 3600.0);
                }
            }
        }

        // Check if vessel intersected the candidate search corridor
        if (min_dist_km <= corridor_radius_km_) {
            AISAttributionReport rep;
            rep.candidate_mmsi = mmsi;
            rep.vessel_name = track.front().vessel_name;
            rep.vessel_type = track.front().vessel_type;
            rep.min_distance_to_origin_km = min_dist_km;
            rep.is_spoofed_teleportation = speed_spoofed;
            rep.is_dark_vessel = dark_gap;
            rep.dark_gap_hours = max_gap_hours;

            // Base spatial-temporal proximity score [0.0, 1.0]
            double spatial_score = exp(-0.5 * (min_dist_km / 5.0) * (min_dist_km / 5.0));
            double time_diff_hours = abs(closest_time_sec - spill_timestamp_epoch_sec) / 3600.0;
            double time_score = exp(-0.5 * (time_diff_hours / 2.0) * (time_diff_hours / 2.0));
            float raw_confidence = static_cast<float>(spatial_score * time_score);

            // Correlate with CFAR Radar Hard Targets (Validating metal hull on satellite)
            for (const auto& r_target : radar_targets) {
                double r_dist = distance_km(r_target.location, {track.front().lon, track.front().lat});
                if (r_dist < 1.5) { // within 1.5 km radar resolution cell
                    rep.matched_radar_hard_target = true;
                    rep.radar_target_id = r_target.id;
                    break;
                }
            }

            // Apply Penalties and Boosts
            float confidence_penalty = 0.0f;
            if (speed_spoofed) {
                confidence_penalty += 0.40f; // Severe penalty for GPS spoofing
            }
            if (dark_gap) {
                // If a dark vessel matches a radar hard target, it is suspicious
                if (rep.matched_radar_hard_target) {
                    raw_confidence = max(raw_confidence, 0.85f);
                } else {
                    confidence_penalty += 0.25f;
                }
            }

            rep.confidence_penalty = confidence_penalty;
            rep.attribution_confidence = clamp(raw_confidence - confidence_penalty, 0.05f, 0.99f);

            ostringstream oss;
            if (rep.is_dark_vessel && rep.matched_radar_hard_target) {
                oss << "CRITICAL: Dark Vessel detected! Transponder silent for " << max_gap_hours 
                    << "h but validated by CFAR Radar Target #" << rep.radar_target_id;
            } else if (rep.is_spoofed_teleportation) {
                oss << "FRAUD WARNING: AIS Speed/Position spoofing detected (>40 knots for displacement hull).";
            } else {
                oss << "Correlated vessel trajectory within " << min_dist_km << " km of spill origin.";
            }
            rep.status_summary = oss.str();

            candidates.push_back(rep);
        }
    }

    // High-Density Chokepoint Check (> 15 vessels sharing corridor)
    if (candidates.size() >= 15) {
        for (auto& cand : candidates) {
            cand.is_high_density_chokepoint = true;
            cand.co_located_vessel_count = candidates.size();
            // Kinematic correlation breakdown penalty in crowded chokepoint
            cand.attribution_confidence *= 0.50f;
            cand.status_summary += " [HIGH-DENSITY CHOKEPOINT: Attribution confidence degraded by 50%]";
        }
    }

    // Sort candidates by attribution confidence descending
    sort(candidates.begin(), candidates.end(), [](const AISAttributionReport& a, const AISAttributionReport& b) {
        return a.attribution_confidence > b.attribution_confidence;
    });

    return candidates;
}

} // namespace sar::attribution
