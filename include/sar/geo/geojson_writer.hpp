#pragma once

#include "sar/geo/geo_types.hpp"
#include "sar/drift/lagrangian_drift.hpp"
#include "sar/attribution/cfar_detector.hpp"
#include "sar/attribution/ais_engine.hpp"
#include <string>
#include <vector>

namespace sar::geo {

class GeoJSONWriter {
public:
    /**
     * @brief Serializes tactical SAR detection layers (OilSlick, ReverseDriftPlume, RadarHardTarget, SuspectVessel)
     * into an RFC 7946 standardized GeoJSON FeatureCollection.
     */
    static std::string serialize(
        const std::vector<SlickFeature>& slicks,
        const std::vector<drift::ReverseDriftResult>& drift_corridors = {},
        const std::vector<attribution::RadarHardTarget>& radar_targets = {},
        const std::vector<attribution::AISAttributionReport>& suspect_vessels = {},
        const std::vector<attribution::AISTransmission>& ais_feed = {});

    /**
     * @brief Writes unified GeoJSON FeatureCollection directly to a file.
     */
    static bool write_file(
        const std::string& filepath,
        const std::vector<SlickFeature>& slicks,
        const std::vector<drift::ReverseDriftResult>& drift_corridors = {},
        const std::vector<attribution::RadarHardTarget>& radar_targets = {},
        const std::vector<attribution::AISAttributionReport>& suspect_vessels = {},
        const std::vector<attribution::AISTransmission>& ais_feed = {});
};

} // namespace sar::geo
