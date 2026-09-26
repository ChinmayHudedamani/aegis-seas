#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace aegis::sar {

struct TiffHeaderInfo {
    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t bits_per_sample = 16;
    uint16_t sample_format = 1; // 1 = unsigned int, 2 = int, 3 = float
    uint32_t tile_width = 512;
    uint32_t tile_height = 512;
    std::vector<uint64_t> strip_offsets;
    std::vector<uint64_t> strip_byte_counts;
    bool is_big_tiff = false;
    bool is_little_endian = true;
};

class ZeroCopySARReader {
public:
    ZeroCopySARReader();
    ~ZeroCopySARReader();

    // Disable copy for RAII memory-mapping safety
    ZeroCopySARReader(const ZeroCopySARReader&) = delete;
    ZeroCopySARReader& operator=(const ZeroCopySARReader&) = delete;

    /**
     * @brief Memory-map a Sentinel-1 GeoTIFF / BigTIFF scene using OS zero-copy primitives.
     */
    bool open_scene(const std::string& filepath);
    void close_scene();

    /**
     * @brief Parse basic TIFF/GeoTIFF metadata tags from mapped memory.
     */
    bool parse_metadata();

    /**
     * @brief Read a 512x512 tile directly from mapped virtual memory into calibrated float dB buffer.
     */
    bool read_calibrated_tile(uint32_t tile_x, uint32_t tile_y,
                              const std::vector<float>& lut_sigma_gain,
                              std::vector<float>& out_tile_db);

    /**
     * @brief 5x5 Enhanced Lee Speckle Filter (in-place execution on 512x512 tile).
     */
    static void apply_enhanced_lee_filter(std::vector<float>& tile_data,
                                         uint32_t width, uint32_t height,
                                         float damping = 1.0f,
                                         uint32_t num_looks = 4);

    const TiffHeaderInfo& get_header() const { return header_; }
    size_t get_mapped_size() const { return file_size_; }

private:
    std::string filepath_;
    const uint8_t* mapped_data_ = nullptr;
    size_t file_size_ = 0;

#if defined(_WIN32) || defined(_WIN64)
    HANDLE hFile_ = INVALID_HANDLE_VALUE;
    HANDLE hMapping_ = NULL;
#else
    int fd_ = -1;
#endif

    TiffHeaderInfo header_;
};

} // namespace aegis::sar
