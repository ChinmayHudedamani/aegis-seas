#pragma once

#include "sar/attribution/ais_engine.hpp"
#include <string>
#include <vector>

namespace sar::io {

class AISParser {
public:
    /**
     * @brief Parses a standard maritime AIS CSV telemetry file into structured transmissions.
     * Expected CSV columns: mmsi,vessel_name,vessel_type,lon,lat,sog,cog,timestamp
     * Supports both Unix epoch seconds and standard timestamp strings.
     */
    static std::vector<attribution::AISTransmission> parse_csv_file(const std::string& filepath);

    /**
     * @brief Parses CSV content directly from an in-memory string.
     */
    static std::vector<attribution::AISTransmission> parse_csv_string(const std::string& csv_content);
};

} // namespace sar::io
