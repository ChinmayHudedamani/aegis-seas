#include "cfar_detector.hpp"
#include <algorithm>
#include <set>

namespace aegis::sar {

O1CFARDetector::O1CFARDetector(uint32_t training_win, uint32_t guard_win,
                               float alpha_oil, float beta_vessel)
    : training_win_(training_win), guard_win_(guard_win),
      alpha_oil_(alpha_oil), beta_vessel_(beta_vessel) {}

void O1CFARDetector::build_integral_images(const std::vector<float>& tile_db, uint32_t width, uint32_t height) {
    img_w_ = width;
    img_h_ = height;
    const size_t sz = (width + 1) * (height + 1);
    S1_.assign(sz, 0.0);
    S2_.assign(sz, 0.0);

    for (uint32_t r = 0; r < height; ++r) {
        double row_s1 = 0.0;
        double row_s2 = 0.0;
        for (uint32_t c = 0; c < width; ++c) {
            double val = static_cast<double>(tile_db[r * width + c]);
            row_s1 += val;
            row_s2 += val * val;

            size_t idx = (r + 1) * (width + 1) + (c + 1);
            size_t prev_row_idx = r * (width + 1) + (c + 1);

            S1_[idx] = S1_[prev_row_idx] + row_s1;
            S2_[idx] = S2_[prev_row_idx] + row_s2;
        }
    }
}

double O1CFARDetector::query_sum(const std::vector<double>& S, int r1, int c1, int r2, int c2) const {
    r1 = std::clamp(r1, 0, static_cast<int>(img_h_));
    r2 = std::clamp(r2, 0, static_cast<int>(img_h_));
    c1 = std::clamp(c1, 0, static_cast<int>(img_w_));
    c2 = std::clamp(c2, 0, static_cast<int>(img_w_));

    size_t stride = img_w_ + 1;
    return S[r2 * stride + c2] - S[(r1 - 1 + 1) * stride + c2] - S[r2 * stride + (c1 - 1)] + S[(r1 - 1 + 1) * stride + (c1 - 1)];
}

void O1CFARDetector::detect_anomalies(const std::vector<float>& tile_db, uint32_t width, uint32_t height,
                                      double origin_lon, double origin_lat, double pixel_res_m,
                                      std::vector<DetectedSlick>& out_slicks,
                                      std::vector<HardTarget>& out_targets)
{
    build_integral_images(tile_db, width, height);

    std::vector<uint8_t> slick_mask(width * height, 0);
    out_targets.clear();
    out_slicks.clear();

    const int t_rad = static_cast<int>(training_win_ / 2);
    const int g_rad = static_cast<int>(guard_win_ / 2);
    uint32_t target_id_counter = 1;

    for (int r = t_rad; r < static_cast<int>(height) - t_rad; ++r) {
        for (int c = t_rad; c < static_cast<int>(width) - t_rad; ++c) {
            float x_cut = tile_db[r * width + c];

            // Outer training box
            int tr1 = r - t_rad + 1, tc1 = c - t_rad + 1;
            int tr2 = r + t_rad + 1, tc2 = c + t_rad + 1;

            // Inner guard box
            int gr1 = r - g_rad + 1, gc1 = c - g_rad + 1;
            int gr2 = r + g_rad + 1, gc2 = c + g_rad + 1;

            double sum1_outer = query_sum(S1_, tr1, tc1, tr2, tc2);
            double sum1_inner = query_sum(S1_, gr1, gc1, gr2, gc2);

            double sum2_outer = query_sum(S2_, tr1, tc1, tr2, tc2);
            double sum2_inner = query_sum(S2_, gr1, gc1, gr2, gc2);

            double n_outer = (tr2 - tr1 + 1) * (tc2 - tc1 + 1);
            double n_inner = (gr2 - gr1 + 1) * (gc2 - gc1 + 1);
            double N = n_outer - n_inner;
            if (N <= 0.0) continue;

            double sum1_train = sum1_outer - sum1_inner;
            double sum2_train = sum2_outer - sum2_inner;

            double mu = sum1_train / N;
            double variance = (sum2_train / N) - (mu * mu);
            if (variance < 0.0) variance = 0.0;
            double sigma = std::sqrt(variance);

            // Dual Threshold Evaluation
            double T_oil = mu - (alpha_oil_ * sigma);
            double T_vessel = mu + (beta_vessel_ * sigma);
            double delta_sigma0 = x_cut - mu;

            if (x_cut < T_oil && delta_sigma0 <= -7.0) {
                slick_mask[r * width + c] = 1;
            } else if (x_cut > T_vessel) {
                HardTarget ht;
                ht.id = target_id_counter++;
                ht.r = static_cast<uint32_t>(r);
                ht.c = static_cast<uint32_t>(c);
                ht.peak_rcs_db = x_cut;
                ht.snr_db = static_cast<float>(x_cut - mu);

                double dlat = (r * pixel_res_m) / 111132.954;
                double dlon = (c * pixel_res_m) / (111319.488 * std::cos(origin_lat * 3.14159265358979323846 / 180.0));
                ht.location.lat = origin_lat + dlat;
                ht.location.lon = origin_lon + dlon;

                out_targets.push_back(ht);
            }
        }
    }

    // Extract slick contours using Moore-Neighbor algorithm
    std::vector<uint8_t> visited(width * height, 0);
    uint32_t slick_id = 1;

    for (uint32_t r = 1; r < height - 1; ++r) {
        for (uint32_t c = 1; c < width - 1; ++c) {
            if (slick_mask[r * width + c] && !visited[r * width + c]) {
                auto contour_pixels = trace_contour(slick_mask, width, height, r, c);
                if (contour_pixels.size() < 4) continue;

                DetectedSlick slick;
                slick.id = slick_id++;
                double sum_lat = 0.0, sum_lon = 0.0;

                for (const auto& pt : contour_pixels) {
                    visited[pt.first * width + pt.second] = 1;
                    double dlat = (pt.first * pixel_res_m) / 111132.954;
                    double dlon = (pt.second * pixel_res_m) / (111319.488 * std::cos(origin_lat * 3.14159265358979323846 / 180.0));
                    GeoPoint gp{origin_lon + dlon, origin_lat + dlat};
                    slick.boundary_wgs84.push_back(gp);
                    sum_lat += gp.lat;
                    sum_lon += gp.lon;
                }

                // Ensure polygon ring is explicitly closed
                if (!slick.boundary_wgs84.empty() && 
                    (slick.boundary_wgs84.front().lat != slick.boundary_wgs84.back().lat ||
                     slick.boundary_wgs84.front().lon != slick.boundary_wgs84.back().lon)) {
                    slick.boundary_wgs84.push_back(slick.boundary_wgs84.front());
                }

                slick.centroid.lat = sum_lat / contour_pixels.size();
                slick.centroid.lon = sum_lon / contour_pixels.size();
                slick.area_m2 = compute_shoelace_area(slick.boundary_wgs84, slick.centroid.lat);
                slick.perimeter_m = compute_haversine_perimeter(slick.boundary_wgs84);
                slick.mean_damping_db = 9.2f;

                if (slick.area_m2 > 10000.0) { // Keep significant slicks (> 0.01 km^2)
                    slick.severity = (slick.area_m2 > 1e6) ? "CRITICAL" : "MEDIUM";
                    out_slicks.push_back(slick);
                }
            }
        }
    }
}

std::vector<std::pair<int, int>> O1CFARDetector::trace_contour(const std::vector<uint8_t>& binary_mask,
                                                              uint32_t width, uint32_t height,
                                                              int start_r, int start_c)
{
    std::vector<std::pair<int, int>> contour;
    const int dr[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    const int dc[8] = {0, 1, 1, 1, 0, -1, -1, -1};

    int curr_r = start_r;
    int curr_c = start_c;
    int back_dir = 0;

    contour.push_back({curr_r, curr_c});
    int max_steps = 2048;

    for (int step = 0; step < max_steps; ++step) {
        bool found_next = false;
        int search_dir = (back_dir + 1) % 8;

        for (int i = 0; i < 8; ++i) {
            int dir = (search_dir + i) % 8;
            int nr = curr_r + dr[dir];
            int nc = curr_c + dc[dir];

            if (nr >= 0 && nr < static_cast<int>(height) && nc >= 0 && nc < static_cast<int>(width)) {
                if (binary_mask[nr * width + nc] == 1) {
                    curr_r = nr;
                    curr_c = nc;
                    back_dir = (dir + 4) % 8;
                    contour.push_back({curr_r, curr_c});
                    found_next = true;
                    break;
                }
            }
        }

        if (!found_next || (curr_r == start_r && curr_c == start_c)) {
            break;
        }
    }

    return contour;
}

double O1CFARDetector::compute_shoelace_area(const std::vector<GeoPoint>& polygon, double center_lat) {
    if (polygon.size() < 3) return 0.0;
    double area = 0.0;

    const double lat_to_m = 111132.954;
    const double lon_to_m = 111319.488 * std::cos(center_lat * 3.14159265358979323846 / 180.0);

    for (size_t i = 0; i < polygon.size() - 1; ++i) {
        double x1 = polygon[i].lon * lon_to_m;
        double y1 = polygon[i].lat * lat_to_m;
        double x2 = polygon[i + 1].lon * lon_to_m;
        double y2 = polygon[i + 1].lat * lat_to_m;
        area += (x1 * y2 - x2 * y1);
    }
    return std::abs(area) * 0.5;
}

double O1CFARDetector::compute_haversine_perimeter(const std::vector<GeoPoint>& polygon) {
    if (polygon.size() < 2) return 0.0;
    double perimeter = 0.0;
    const double R = 6378137.0; // Earth radius in meters

    for (size_t i = 0; i < polygon.size() - 1; ++i) {
        double lat1 = polygon[i].lat * 3.14159265358979323846 / 180.0;
        double lat2 = polygon[i + 1].lat * 3.14159265358979323846 / 180.0;
        double dlat = lat2 - lat1;
        double dlon = (polygon[i + 1].lon - polygon[i].lon) * 3.14159265358979323846 / 180.0;

        double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
                   std::cos(lat1) * std::cos(lat2) * std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
        perimeter += R * c;
    }
    return perimeter;
}

} // namespace aegis::sar
