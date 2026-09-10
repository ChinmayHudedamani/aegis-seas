#include "sar/io/ais_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

namespace sar::io {

static inline string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\"");
    return str.substr(first, (last - first + 1));
}

vector<attribution::AISTransmission> AISParser::parse_csv_string(const string& csv_content) {
    vector<attribution::AISTransmission> records;
    istringstream stream(csv_content);
    string line;
    bool is_first_line = true;

    while (getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        // Split by comma
        vector<string> tokens;
        string token;
        istringstream line_stream(line);
        while (getline(line_stream, token, ',')) {
            tokens.push_back(trim(token));
        }

        if (tokens.size() < 8) continue;

        // Check for header row
        if (is_first_line) {
            is_first_line = false;
            string col0_lower = tokens[0];
            transform(col0_lower.begin(), col0_lower.end(), col0_lower.begin(), ::tolower);
            if (col0_lower == "mmsi" || col0_lower.find("mmsi") != string::npos) {
                continue;
            }
        }

        try {
            attribution::AISTransmission tx;
            tx.mmsi = tokens[0];
            tx.vessel_name = tokens[1];
            tx.vessel_type = tokens[2];
            tx.lon = stod(tokens[3]);
            tx.lat = stod(tokens[4]);
            tx.speed_knots = stof(tokens[5]);
            tx.course_deg = stof(tokens[6]);

            // Timestamp parsing: numeric epoch or fallback
            try {
                tx.timestamp_epoch_sec = stoll(tokens[7]);
            } catch (...) {
                tx.timestamp_epoch_sec = 1700000000;
            }

            records.push_back(tx);
        } catch (const exception&) {
            // Skip corrupt line
            continue;
        }
    }

    return records;
}

vector<attribution::AISTransmission> AISParser::parse_csv_file(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "[AISParser] Warning: Failed to open AIS telemetry file: " << filepath << "\n";
        return {};
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return parse_csv_string(buffer.str());
}

} // namespace sar::io
