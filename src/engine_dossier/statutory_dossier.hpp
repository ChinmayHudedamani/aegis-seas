#pragma once

#include "sar/geo/geo_types.hpp"
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <cstdint>

namespace aegis::dossier {

struct BonnVolumetricReport {
    double total_volume_m3 = 0.0;
    double total_volume_liters = 0.0;
    double total_mass_metric_tons = 0.0;
    double sheen_area_m2 = 0.0;     // Band 1: 0.08 um
    double rainbow_area_m2 = 0.0;   // Band 2: 2.5 um
    double true_oil_area_m2 = 0.0;  // Band 3: 25.0 um
    double emulsion_area_m2 = 0.0;  // Band 4: 200.0 um
};

struct MARPOLEvaluation {
    double discharge_rate_liters_per_nmi = 0.0;
    bool is_statutory_violation = false;
    std::string legal_classification = "";
    std::string unclos_article_reference = "UNCLOS Article 217 (Enforcement by Flag States)";
    std::string marpol_reg_reference = "MARPOL 73/78 Annex I Regulation 15";
};

struct CryptographicSeal {
    std::string sha256_scene_hash;
    std::string sha256_payload_hash;
    std::string ecdsa_p256_signature_hex;
    std::string timestamp_utc;
    bool is_tamper_proof = true;
};

class StatutoryDossierGenerator {
public:
    StatutoryDossierGenerator(double crude_density_kg_m3 = 860.0);

    /**
     * @brief Partition slick pixels into Bonn Agreement Oil Appearance Code (BAOAC) thickness bands.
     */
    BonnVolumetricReport calculate_bonn_volumetrics(
        double total_area_m2, float mean_damping_db) const;

    /**
     * @brief Evaluate statutory violation threshold against MARPOL 73/78 Annex I Regulation 15 (Q > 30 L/NM).
     */
    MARPOLEvaluation evaluate_marpol_breach(
        const BonnVolumetricReport& volumetrics, double slick_length_meters) const;

    /**
     * @brief Calculate SHA-256 digest over string payload.
     */
    static std::string compute_sha256(const std::string& input);

    /**
     * @brief Generate ECDSA NIST P-256 digital signature hex for court admissibility.
     */
    static std::string generate_ecdsa_p256_signature(const std::string& sha256_hash);

    /**
     * @brief Construct canonical JSON-LD statutory dossier document.
     */
    std::string generate_json_dossier(
        const std::string& case_ref,
        const sar::geo::GeoPoint& slick_centroid,
        const BonnVolumetricReport& volumetrics,
        const MARPOLEvaluation& marpol,
        const std::string& suspect_mmsi,
        const std::string& suspect_name,
        double attribution_confidence,
        const CryptographicSeal& seal) const;
private:
    double rho_oil_;
};

} // namespace aegis::dossier
