#include "sar/geo/slick_analyzer.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

namespace sar::geo {

SlickAnalyzer::SlickAnalyzer(const GeoTransform& transform)
    : transform_(transform) {}

vector<SlickFeature> SlickAnalyzer::extract_slicks(
    const core::Matrix2D<uint8_t>& mask,
    const core::Matrix2D<float>& vv_db,
    const core::Matrix2D<float>& vh_db,
    float wind_speed_mps,
    float wind_dir_deg) const 
{
    const size_t rows = mask.rows();
    const size_t cols = mask.cols();
    core::Matrix2D<uint8_t> visited(rows, cols, 0);

    vector<SlickFeature> slicks;
    size_t feature_id = 1;

    const int dr[] = {-1, -1, -1,  0, 0,  1, 1, 1};
    const int dc[] = {-1,  0,  1, -1, 1, -1, 0, 1};

    for (size_t r = 1; r < rows - 1; ++r) {
        for (size_t c = 1; c < cols - 1; ++c) {
            if (mask(r, c) == 1 && visited(r, c) == 0) {
                vector<pair<int, int>> component_pixels;
                vector<pair<int, int>> queue;
                queue.push_back({static_cast<int>(r), static_cast<int>(c)});
                visited(r, c) = 1;

                while (!queue.empty()) {
                    auto [cr, cc] = queue.back();
                    queue.pop_back();
                    component_pixels.push_back({cr, cc});

                    for (int i = 0; i < 8; ++i) {
                        int nr = cr + dr[i];
                        int nc = cc + dc[i];
                        if (nr >= 0 && nr < static_cast<int>(rows) && 
                            nc >= 0 && nc < static_cast<int>(cols)) {
                            if (mask(nr, nc) == 1 && visited(nr, nc) == 0) {
                                visited(nr, nc) = 1;
                                queue.push_back({nr, nc});
                            }
                        }
                    }
                }

                if (component_pixels.size() < 25) {
                    continue;
                }

                core::Matrix2D<uint8_t> comp_mask(rows, cols, 0);
                for (const auto& [pr, pc] : component_pixels) {
                    comp_mask(pr, pc) = 1;
                }

                auto profile = physics::PolarimetricAnalyzer::analyze_candidate(vv_db, vh_db, comp_mask);

                SlickFeature feature = compute_feature_metrics(
                    feature_id++, component_pixels, comp_mask, profile, wind_speed_mps, wind_dir_deg);
                slicks.push_back(feature);
            }
        }
    }

    return slicks;
}

GeoPoint SlickAnalyzer::pixel_to_geo(double r, double c) const {
    constexpr double R_earth = 6378137.0;
    constexpr double deg_to_rad = 3.141592653589793 / 180.0;
    constexpr double rad_to_deg = 180.0 / 3.141592653589793;

    double lat_rad = transform_.origin_lat * deg_to_rad;
    double d_x = c * transform_.pixel_size_meters;
    double d_y = r * transform_.pixel_size_meters;

    double lon = transform_.origin_lon + (d_x / (R_earth * cos(lat_rad))) * rad_to_deg;
    double lat = transform_.origin_lat - (d_y / R_earth) * rad_to_deg;
    return {lon, lat};
}

vector<pair<double, double>> SlickAnalyzer::simplify_rdp(
    const vector<pair<double, double>>& points,
    double epsilon) 
{
    if (points.size() < 3) return points;

    auto perp_dist = [](const pair<double, double>& p,
                        const pair<double, double>& a,
                        const pair<double, double>& b) -> double {
        double dx = b.first - a.first;
        double dy = b.second - a.second;
        double norm_sq = dx * dx + dy * dy;
        if (norm_sq < 1e-9) {
            double px = p.first - a.first;
            double py = p.second - a.second;
            return sqrt(px * px + py * py);
        }
        double num = abs(dy * p.first - dx * p.second + b.first * a.second - b.second * a.first);
        return num / sqrt(norm_sq);
    };

    double d_max = 0.0;
    size_t index = 0;
    size_t last = points.size() - 1;

    for (size_t i = 1; i < last; ++i) {
        double d = perp_dist(points[i], points.front(), points.back());
        if (d > d_max) {
            d_max = d;
            index = i;
        }
    }

    if (d_max > epsilon) {
        vector<pair<double, double>> left(points.begin(), points.begin() + index + 1);
        vector<pair<double, double>> right(points.begin() + index, points.end());

        auto res_left = simplify_rdp(left, epsilon);
        auto res_right = simplify_rdp(right, epsilon);

        res_left.pop_back(); // Remove duplicate junction point
        res_left.insert(res_left.end(), res_right.begin(), res_right.end());
        return res_left;
    }

    return {points.front(), points.back()};
}

vector<GeoPoint> SlickAnalyzer::trace_moore_contour(
    const core::Matrix2D<uint8_t>& comp_mask,
    int min_r, int max_r, int min_c, int max_c) const 
{
    const size_t rows = comp_mask.rows();
    const size_t cols = comp_mask.cols();

    // 1. Locate starting pixel (top-leftmost foreground pixel)
    pair<int, int> start = {-1, -1};
    for (int r = min_r; r <= max_r; ++r) {
        for (int c = min_c; c <= max_c; ++c) {
            if (comp_mask(r, c) == 1) {
                start = {r, c};
                break;
            }
        }
        if (start.first != -1) break;
    }

    if (start.first == -1) return {};

    // 8-connected clockwise neighborhood
    // 0: N, 1: NE, 2: E, 3: SE, 4: S, 5: SW, 6: W, 7: NW
    const int dr[8] = {-1, -1,  0,  1,  1,  1,  0, -1};
    const int dc[8] = { 0,  1,  1,  1,  0, -1, -1, -1};

    vector<pair<double, double>> raw_contour;
    pair<int, int> current = start;
    int check_dir = 7; // Since start is top-left, came from W (6), next check is (6+1)=7 (NW)
    pair<int, int> second = {-1, -1};
    size_t max_steps = 10000;

    raw_contour.push_back({static_cast<double>(current.first), static_cast<double>(current.second)});

    for (size_t step = 0; step < max_steps; ++step) {
        int next_dir = -1;
        for (int i = 0; i < 8; ++i) {
            int d = (check_dir + i) % 8;
            int nr = current.first + dr[d];
            int nc = current.second + dc[d];
            if (nr >= 0 && nr < static_cast<int>(rows) && 
                nc >= 0 && nc < static_cast<int>(cols) && 
                comp_mask(nr, nc) == 1) {
                next_dir = d;
                break;
            }
        }

        if (next_dir == -1) break; // Isolated point

        pair<int, int> next_pixel = {current.first + dr[next_dir], current.second + dc[next_dir]};

        if (raw_contour.size() == 1) {
            second = next_pixel;
        } else if (current == start && next_pixel == second) {
            // Jacob's stopping condition: closed loop complete
            break;
        }

        raw_contour.push_back({static_cast<double>(next_pixel.first), static_cast<double>(next_pixel.second)});
        current = next_pixel;
        // Backtrack direction into current is (next_dir + 4) % 8
        // Start scanning clockwise from (backtrack + 1)
        check_dir = (next_dir + 5) % 8;
    }

    // If tracing failed to form a loop, fallback to bounding box ring
    if (raw_contour.size() < 3) {
        raw_contour.clear();
        raw_contour.push_back({static_cast<double>(min_r), static_cast<double>(min_c)});
        raw_contour.push_back({static_cast<double>(min_r), static_cast<double>(max_c)});
        raw_contour.push_back({static_cast<double>(max_r), static_cast<double>(max_c)});
        raw_contour.push_back({static_cast<double>(max_r), static_cast<double>(min_c)});
    }

    // 2. Simplify using RDP (epsilon = 1.0 pixel tolerance)
    auto simplified = simplify_rdp(raw_contour, 1.0);

    // 3. Project to WGS84 GeoPoints
    vector<GeoPoint> geo_ring;
    geo_ring.reserve(simplified.size() + 1);
    for (const auto& pt : simplified) {
        geo_ring.push_back(pixel_to_geo(pt.first, pt.second));
    }

    // Ensure strictly closed linear ring conforming to RFC 7946
    if (!geo_ring.empty() && 
        (geo_ring.front().lat != geo_ring.back().lat || geo_ring.front().lon != geo_ring.back().lon)) {
        geo_ring.push_back(geo_ring.front());
    }

    return geo_ring;
}

SlickFeature SlickAnalyzer::compute_feature_metrics(
    size_t id,
    const vector<pair<int, int>>& pixels,
    const core::Matrix2D<uint8_t>& comp_mask,
    const physics::SlickPolarimetricProfile& profile,
    float wind_speed_mps,
    float wind_dir_deg) const 
{
    SlickFeature feat;
    feat.id = id;

    // 1. Centroid and Second Central Moments
    double sum_r = 0.0, sum_c = 0.0;
    int min_r = 1e9, max_r = -1e9, min_c = 1e9, max_c = -1e9;
    for (const auto& [r, c] : pixels) {
        sum_r += r;
        sum_c += c;
        min_r = min(min_r, r);
        max_r = max(max_r, r);
        min_c = min(min_c, c);
        max_c = max(max_c, c);
    }
    double mean_r = sum_r / pixels.size();
    double mean_c = sum_c / pixels.size();
    feat.centroid = pixel_to_geo(mean_r, mean_c);

    double mu20 = 0.0, mu02 = 0.0, mu11 = 0.0;
    for (const auto& [r, c] : pixels) {
        double dr = r - mean_r;
        double dc = c - mean_c;
        mu20 += dr * dr;
        mu02 += dc * dc;
        mu11 += dr * dc;
    }
    mu20 /= pixels.size();
    mu02 /= pixels.size();
    mu11 /= pixels.size();

    double common = sqrt((mu20 - mu02) * (mu20 - mu02) + 4.0 * mu11 * mu11);
    double lambda1 = (mu20 + mu02 + common) / 2.0;
    double lambda2 = max(1e-4, (mu20 + mu02 - common) / 2.0);

    feat.aspect_ratio = sqrt(lambda1 / lambda2);
    feat.orientation_deg = 0.5 * atan2(2.0 * mu11, mu20 - mu02) * (180.0 / 3.141592653589793);

    // 2. Physical Area (km^2) and Perimeter
    double pixel_area_m2 = transform_.pixel_size_meters * transform_.pixel_size_meters;
    feat.area_km2 = (pixels.size() * pixel_area_m2) / 1.0e6;
    feat.perimeter_km = (sqrt(static_cast<double>(pixels.size())) * 4.0 * transform_.pixel_size_meters) / 1000.0;

    // 3. Polarimetric Classification & Severity
    feat.confidence_score = profile.slick_probability;
    feat.mean_damping_db = profile.clutter_vv_db - profile.mean_vv_db;
    feat.mean_contrast_db = profile.contrast_to_clutter_ratio_db;

    if (profile.is_look_alike) {
        feat.classification = "BIOGENIC_LOOK_ALIKE";
        feat.severity = "LOW";
    } else if (feat.aspect_ratio > 4.5) {
        feat.classification = "VESSEL_DISCHARGE";
        feat.severity = (feat.area_km2 > 5.0) ? "CRITICAL" : "MEDIUM";
    } else {
        feat.classification = "MINERAL_OIL_SPILL";
        feat.severity = (feat.area_km2 > 10.0) ? "CRITICAL" : (feat.area_km2 > 2.0 ? "MEDIUM" : "LOW");
    }

    // 4. 24-Hour Fay Drift Vector
    constexpr double deg_to_rad = 3.141592653589793 / 180.0;
    double wind_rad = wind_dir_deg * deg_to_rad;
    double drift_speed_mps = 0.03 * wind_speed_mps;
    double drift_distance_meters_24h = drift_speed_mps * 86400.0;

    double drift_dx = drift_distance_meters_24h * sin(wind_rad);
    double drift_dy = -drift_distance_meters_24h * cos(wind_rad);

    constexpr double R_earth = 6378137.0;
    double lat_rad = feat.centroid.lat * deg_to_rad;
    double drift_lon = feat.centroid.lon + (drift_dx / (R_earth * cos(lat_rad))) * (180.0 / 3.141592653589793);
    double drift_lat = feat.centroid.lat - (drift_dy / R_earth) * (180.0 / 3.141592653589793);
    feat.drift_vector_24h = {drift_lon, drift_lat};

    // 5. True Moore-Neighbor Boundary Tracing + RDP simplification
    feat.boundary = trace_moore_contour(comp_mask, min_r, max_r, min_c, max_c);

    return feat;
}

} // namespace sar::geo
