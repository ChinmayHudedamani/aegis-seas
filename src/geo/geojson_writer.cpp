#include "sar/geo/geojson_writer.hpp"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <map>

using namespace std;

namespace sar::geo {

string GeoJSONWriter::serialize(
    const vector<SlickFeature>& slicks,
    const vector<drift::ReverseDriftResult>& drift_corridors,
    const vector<attribution::RadarHardTarget>& radar_targets,
    const vector<attribution::AISAttributionReport>& suspect_vessels,
    const vector<attribution::AISTransmission>& ais_feed)
{
    ostringstream ss;
    ss << fixed << setprecision(6);
    ss << "{\n";
    ss << "  \"type\": \"FeatureCollection\",\n";
    ss << "  \"features\": [\n";

    bool first_feature = true;
    auto prefix_comma = [&]() {
        if (!first_feature) ss << ",\n";
        first_feature = false;
    };

    // 1. Layer: OilSlick Polygons
    for (const auto& s : slicks) {
        prefix_comma();
        ss << "    {\n";
        ss << "      \"type\": \"Feature\",\n";
        ss << "      \"id\": " << s.id << ",\n";
        ss << "      \"properties\": {\n";
        ss << "        \"feature_type\": \"OilSlick\",\n";
        ss << "        \"classification\": \"" << s.classification << "\",\n";
        ss << "        \"severity\": \"" << s.severity << "\",\n";
        ss << "        \"confidence\": " << s.confidence_score << ",\n";
        ss << "        \"area_km2\": " << s.area_km2 << ",\n";
        ss << "        \"perimeter_km\": " << s.perimeter_km << ",\n";
        ss << "        \"aspect_ratio\": " << s.aspect_ratio << ",\n";
        ss << "        \"orientation_deg\": " << s.orientation_deg << ",\n";
        ss << "        \"mean_damping_db\": " << s.mean_damping_db << ",\n";
        ss << "        \"mean_contrast_db\": " << s.mean_contrast_db << ",\n";
        ss << "        \"centroid_lon\": " << s.centroid.lon << ",\n";
        ss << "        \"centroid_lat\": " << s.centroid.lat << ",\n";
        ss << "        \"drift_24h_lon\": " << s.drift_vector_24h.lon << ",\n";
        ss << "        \"drift_24h_lat\": " << s.drift_vector_24h.lat << "\n";
        ss << "      },\n";
        ss << "      \"geometry\": {\n";
        ss << "        \"type\": \"Polygon\",\n";
        ss << "        \"coordinates\": [[\n";

        for (size_t v = 0; v < s.boundary.size(); ++v) {
            ss << "          [" << s.boundary[v].lon << ", " << s.boundary[v].lat << "]";
            if (v + 1 < s.boundary.size()) ss << ",\n";
            else ss << "\n";
        }

        ss << "        ]]\n";
        ss << "      }\n";
        ss << "    }";
    }

    // 2. Layer: ReverseDriftPlume 95% Confidence Corridor & Estimated Origin
    for (size_t i = 0; i < drift_corridors.size(); ++i) {
        const auto& drift = drift_corridors[i];
        if (drift.confidence_polygon_95.empty()) continue;

        prefix_comma();
        ss << "    {\n";
        ss << "      \"type\": \"Feature\",\n";
        ss << "      \"id\": " << (1000 + i + 1) << ",\n";
        ss << "      \"properties\": {\n";
        ss << "        \"feature_type\": \"ReverseDriftPlume\",\n";
        ss << "        \"corridor_area_km2\": " << drift.corridor_area_km2 << ",\n";
        ss << "        \"origin_lon\": " << drift.estimated_origin.lon << ",\n";
        ss << "        \"origin_lat\": " << drift.estimated_origin.lat << ",\n";
        ss << "        \"excessive_uncertainty\": " << (drift.excessive_uncertainty_warning ? "true" : "false") << ",\n";
        ss << "        \"warning_message\": \"" << drift.warning_message << "\"\n";
        ss << "      },\n";
        ss << "      \"geometry\": {\n";
        ss << "        \"type\": \"Polygon\",\n";
        ss << "        \"coordinates\": [[\n";

        for (size_t v = 0; v < drift.confidence_polygon_95.size(); ++v) {
            ss << "          [" << drift.confidence_polygon_95[v].lon << ", " << drift.confidence_polygon_95[v].lat << "]";
            if (v + 1 < drift.confidence_polygon_95.size()) ss << ",\n";
            else ss << "\n";
        }

        ss << "        ]]\n";
        ss << "      }\n";
        ss << "    }";

        // Reconstructed origin point
        prefix_comma();
        ss << "    {\n";
        ss << "      \"type\": \"Feature\",\n";
        ss << "      \"id\": " << (1100 + i + 1) << ",\n";
        ss << "      \"properties\": {\n";
        ss << "        \"feature_type\": \"ReconstructedOrigin\",\n";
        ss << "        \"corridor_id\": " << (1000 + i + 1) << ",\n";
        ss << "        \"origin_lon\": " << drift.estimated_origin.lon << ",\n";
        ss << "        \"origin_lat\": " << drift.estimated_origin.lat << "\n";
        ss << "      },\n";
        ss << "      \"geometry\": {\n";
        ss << "        \"type\": \"Point\",\n";
        ss << "        \"coordinates\": [" << drift.estimated_origin.lon << ", " << drift.estimated_origin.lat << "]\n";
        ss << "      }\n";
        ss << "    }";
    }

    // 3. Layer: RadarHardTarget (CA-CFAR Hits)
    for (const auto& target : radar_targets) {
        prefix_comma();
        ss << "    {\n";
        ss << "      \"type\": \"Feature\",\n";
        ss << "      \"id\": " << (2000 + target.id) << ",\n";
        ss << "      \"properties\": {\n";
        ss << "        \"feature_type\": \"RadarHardTarget\",\n";
        ss << "        \"target_id\": " << target.id << ",\n";
        ss << "        \"pixel_r\": " << target.pixel_r << ",\n";
        ss << "        \"pixel_c\": " << target.pixel_c << ",\n";
        ss << "        \"peak_rcs_db\": " << target.peak_rcs_db << ",\n";
        ss << "        \"snr_db\": " << target.snr_db << ",\n";
        ss << "        \"has_correlated_ais\": " << (target.has_correlated_ais ? "true" : "false") << ",\n";
        ss << "        \"correlated_mmsi\": \"" << target.correlated_mmsi << "\"\n";
        ss << "      },\n";
        ss << "      \"geometry\": {\n";
        ss << "        \"type\": \"Point\",\n";
        ss << "        \"coordinates\": [" << target.location.lon << ", " << target.location.lat << "]\n";
        ss << "      }\n";
        ss << "    }";
    }

    // Index AIS feed by MMSI for track generation
    map<string, vector<attribution::AISTransmission>> tracks;
    for (const auto& ping : ais_feed) {
        tracks[ping.mmsi].push_back(ping);
    }

    // 4. Layer: SuspectVessel Candidates & Tracks
    for (size_t i = 0; i < suspect_vessels.size(); ++i) {
        const auto& rep = suspect_vessels[i];
        const auto& track = tracks[rep.candidate_mmsi];

        prefix_comma();
        ss << "    {\n";
        ss << "      \"type\": \"Feature\",\n";
        ss << "      \"id\": " << (3000 + i + 1) << ",\n";
        ss << "      \"properties\": {\n";
        ss << "        \"feature_type\": \"SuspectVessel\",\n";
        ss << "        \"candidate_mmsi\": \"" << rep.candidate_mmsi << "\",\n";
        ss << "        \"vessel_name\": \"" << rep.vessel_name << "\",\n";
        ss << "        \"vessel_type\": \"" << rep.vessel_type << "\",\n";
        ss << "        \"attribution_confidence\": " << rep.attribution_confidence << ",\n";
        ss << "        \"min_distance_to_origin_km\": " << rep.min_distance_to_origin_km << ",\n";
        ss << "        \"is_dark_vessel\": " << (rep.is_dark_vessel ? "true" : "false") << ",\n";
        ss << "        \"dark_gap_hours\": " << rep.dark_gap_hours << ",\n";
        ss << "        \"is_spoofed_teleportation\": " << (rep.is_spoofed_teleportation ? "true" : "false") << ",\n";
        ss << "        \"is_high_density_chokepoint\": " << (rep.is_high_density_chokepoint ? "true" : "false") << ",\n";
        ss << "        \"matched_radar_hard_target\": " << (rep.matched_radar_hard_target ? "true" : "false") << ",\n";
        ss << "        \"radar_target_id\": " << rep.radar_target_id << ",\n";
        ss << "        \"status_summary\": \"" << rep.status_summary << "\"\n";
        ss << "      },\n";

        if (track.size() >= 2) {
            ss << "      \"geometry\": {\n";
            ss << "        \"type\": \"LineString\",\n";
            ss << "        \"coordinates\": [\n";
            for (size_t t = 0; t < track.size(); ++t) {
                ss << "          [" << track[t].lon << ", " << track[t].lat << "]";
                if (t + 1 < track.size()) ss << ",\n";
                else ss << "\n";
            }
            ss << "        ]\n";
            ss << "      }\n";
        } else if (!track.empty()) {
            ss << "      \"geometry\": {\n";
            ss << "        \"type\": \"Point\",\n";
            ss << "        \"coordinates\": [" << track.front().lon << ", " << track.front().lat << "]\n";
            ss << "      }\n";
        } else {
            // Fallback point
            ss << "      \"geometry\": {\n";
            ss << "        \"type\": \"Point\",\n";
            ss << "        \"coordinates\": [0.0, 0.0]\n";
            ss << "      }\n";
        }
        ss << "    }";
    }

    ss << "\n  ]\n";
    ss << "}\n";
    return ss.str();
}

bool GeoJSONWriter::write_file(
    const string& filepath,
    const vector<SlickFeature>& slicks,
    const vector<drift::ReverseDriftResult>& drift_corridors,
    const vector<attribution::RadarHardTarget>& radar_targets,
    const vector<attribution::AISAttributionReport>& suspect_vessels,
    const vector<attribution::AISTransmission>& ais_feed)
{
    ofstream out(filepath);
    if (!out.is_open()) {
        return false;
    }
    out << serialize(slicks, drift_corridors, radar_targets, suspect_vessels, ais_feed);
    out.close();
    return true;
}

} // namespace sar::geo
