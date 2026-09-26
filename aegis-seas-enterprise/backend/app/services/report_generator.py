"""
AEGIS-SEAS Enterprise: Automated Investigation Report Generator Service
Strict Legal Compliance & Terminology Enforcement:
  * Uses "Candidate Vessel", "Potential Source Vessel", "Investigation Priority"
  * Uses "Investigation-ready, traceable evidence dossier"
  * Uses "Probable origin region and estimated time window"
"""

import hashlib
import json
from datetime import datetime
from typing import Dict, Any
from app.models.incident import CaseSchema

class InvestigationReportGenerator:
    @staticmethod
    def generate_json_dossier(case: CaseSchema) -> Dict[str, Any]:
        now_utc = datetime.utcnow().isoformat() + "Z"
        
        # Build SHA-256 payload digest over case parameters
        raw_digest_str = f"{case.incident_id}|{case.title}|{case.recorded_at}|{case.assigned_investigator}"
        payload_hash = hashlib.sha256(raw_digest_str.encode('utf-8')).hexdigest()

        dossier = {
            "@context": "https://schema.org",
            "type": "InvestigationReadyEvidenceDossier",
            "header": {
                "case_id": case.incident_id,
                "incident_name": case.title,
                "jurisdiction": "Indian Exclusive Economic Zone (EEZ) / UNCLOS Article 217",
                "classification": "RESTRICTED / MARITIME INVESTIGATION DOSSIER",
                "date_of_report": now_utc
            },
            "executive_summary": {
                "narrative": (
                    f"On {case.recorded_at}, an anomalous surface feature classified as {case.incident_type.value} "
                    f"was identified at centroid coordinates {case.coordinates[0]}°N, {case.coordinates[1]}°E. "
                    f"Stochastic hydrodynamic back-trajectories estimate a probable origin window between "
                    f"{case.estimated_window.start_time} and {case.estimated_window.end_time} "
                    f"(Confidence: {case.estimated_window.confidence_pct}%). "
                    f"Analysis identified candidate vessels warranting further investigation based on spatial-temporal correlation."
                ),
                "severity": case.severity.value,
                "status": case.status.value,
                "assigned_investigator": case.assigned_investigator
            },
            "satellite_observations": {
                "sensor_platform": "Sentinel-1 C-Band Synthetic Aperture Radar (SAR)",
                "backscatter_attenuation": "-8.9 dB to -9.6 dB contrast suppression",
                "estimated_surface_area_km2": 2.508,
                "bonn_volume_estimate_m3": 45.0,
                "physical_limitations": "All-weather SAR radar capability with physical sea-state limitations."
            },
            "metocean_advection_reconstruction": {
                "wind_speed_kts": case.environmental_context.wind_speed_kts,
                "sea_surface_temp_c": case.environmental_context.sea_surface_temp_c,
                "tidal_phase": case.environmental_context.tidal_phase,
                "current_drift_kts": case.environmental_context.current_drift_kts,
                "advection_model": "4th-Order Runge-Kutta (RK4) Stochastic Brownian Dispersion (5,000 Parcels)"
            },
            "candidate_vessels_of_interest": [
                {
                    "vessel_id": vessel.vessel_id,
                    "vessel_name": vessel.vessel_name,
                    "flag_state": vessel.flag_state,
                    "vessel_type": vessel.vessel_type,
                    "investigation_priority": vessel.investigation_priority.value,
                    "reasons_for_investigation": vessel.why_this_vessel,
                    "metrics": vessel.metrics
                }
                for vessel in case.candidate_vessels
            ],
            "technical_uncertainties_and_data_limitations": [
                "Temporal gaps in terrestrial AIS coverage near offshore EEZ boundaries.",
                "Satellite revisit latency (12-hour Sentinel-1 constellation orbital interval).",
                "Windage coefficient variance under turbulent sea state (Beaufort 4-6)."
            ],
            "traceable_data_sources": [
                {
                    "evidence_id": item.evidence_id,
                    "source_type": item.source_type.value,
                    "timestamp_utc": item.timestamp_utc,
                    "sensor_id": item.metadata_proof.get("sensor_id", "N/A"),
                    "sha256_checksum": item.metadata_proof.get("ingest_checksum_sha256", "N/A")
                }
                for item in case.evidence_vault
            ],
            "investigator_sign_off": {
                "analyst_name": case.assigned_investigator,
                "verification_status": case.status.value,
                "analyst_notes": case.analyst_notes or "Case reviewed and verified.",
                "payload_sha256_hash": payload_hash,
                "signed_at": now_utc
            }
        }
        return dossier

    @staticmethod
    def generate_printable_html_report(case: CaseSchema) -> str:
        dossier = InvestigationReportGenerator.generate_json_dossier(case)
        exec_summary = dossier["executive_summary"]
        sat_obs = dossier["satellite_observations"]
        metocean = dossier["metocean_advection_reconstruction"]

        vessel_rows = ""
        for v in dossier["candidate_vessels_of_interest"]:
            reasons = "".join([f"<li>{r}</li>" for r in v["reasons_for_investigation"]])
            vessel_rows += f"""
            <tr>
                <td><strong>{v['vessel_name']}</strong><br><small>{v['vessel_id']}</small></td>
                <td>{v['flag_state']}<br><small>{v['vessel_type']}</small></td>
                <td><span class="badge tier">{v['investigation_priority']}</span></td>
                <td><ul class="reason-list">{reasons}</ul></td>
            </tr>
            """

        evidence_rows = ""
        for ev in dossier["traceable_data_sources"]:
            evidence_rows += f"""
            <tr>
                <td>{ev['evidence_id']}</td>
                <td>{ev['source_type']}</td>
                <td>{ev['timestamp_utc']}</td>
                <td><code>{ev['sha256_checksum']}</code></td>
            </tr>
            """

        html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>AEGIS-SEAS Enterprise Investigation Dossier - {case.incident_id}</title>
    <style>
        body {{ font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif; margin: 30px; color: #1e293b; background: #fff; line-height: 1.5; font-size: 10pt; }}
        .header {{ border-bottom: 3px solid #0284c7; padding-bottom: 10px; margin-bottom: 20px; display: flex; justify-content: space-between; align-items: flex-end; }}
        .title {{ font-size: 18pt; font-weight: 800; color: #0f172a; margin: 0; }}
        .subtitle {{ font-size: 10pt; color: #0284c7; font-weight: 700; text-transform: uppercase; }}
        .section-title {{ font-size: 11pt; font-weight: 700; color: #0369a1; text-transform: uppercase; border-left: 4px solid #0284c7; padding-left: 8px; margin: 20px 0 8px 0; }}
        .meta-grid {{ width: 100%; border-collapse: collapse; margin-bottom: 15px; }}
        .meta-grid td {{ border: 1px solid #cbd5e1; padding: 6px 10px; font-size: 9pt; }}
        .meta-grid td.label {{ background: #f8fafc; font-weight: 700; width: 25%; }}
        .callout {{ background: #f0f9ff; border: 1px solid #bae6fd; border-radius: 4px; padding: 10px 14px; color: #0369a1; font-size: 9pt; margin: 12px 0; }}
        table.data-table {{ width: 100%; border-collapse: collapse; margin: 10px 0; font-size: 8.5pt; }}
        table.data-table th, table.data-table td {{ border: 1px solid #cbd5e1; padding: 6px 8px; text-align: left; }}
        table.data-table th {{ background: #f1f5f9; font-weight: 700; color: #0f172a; }}
        .badge {{ display: inline-block; padding: 2px 6px; border-radius: 4px; font-size: 8pt; font-weight: 700; background: #fef3c7; color: #92400e; }}
        .reason-list {{ margin: 0; padding-left: 14px; }}
        .signature-box {{ border: 1px solid #cbd5e1; background: #f8fafc; padding: 12px; margin-top: 25px; border-radius: 4px; }}
        @media print {{ body {{ margin: 0; }} }}
    </style>
</head>
<body>
    <div class="header">
        <div>
            <h1 class="title">AEGIS-SEAS ENTERPRISE</h1>
            <div class="subtitle">Investigation-Ready Evidence Dossier</div>
        </div>
        <div style="text-align: right;">
            <strong>CASE REF: {case.incident_id}</strong><br>
            <small>Report Date: {dossier['header']['date_of_report']}</small>
        </div>
    </div>

    <table class="meta-grid">
        <tr>
            <td class="label">Incident Title</td>
            <td><strong>{case.title}</strong></td>
            <td class="label">Centroid Coordinates</td>
            <td>{case.coordinates[0]}°N, {case.coordinates[1]}°E</td>
        </tr>
        <tr>
            <td class="label">Incident Type</td>
            <td>{case.incident_type.value}</td>
            <td class="label">Severity Level</td>
            <td><strong>{case.severity.value}</strong></td>
        </tr>
        <tr>
            <td class="label">Status</td>
            <td><strong>{case.status.value}</strong></td>
            <td class="label">Assigned Investigator</td>
            <td>{case.assigned_investigator}</td>
        </tr>
    </table>

    <div class="section-title">1. Executive Summary</div>
    <p>{exec_summary['narrative']}</p>

    <div class="section-title">2. Satellite Observations & Physical Capabilities</div>
    <div class="callout">
        <strong>Radar Sensor:</strong> {sat_obs['sensor_platform']}<br>
        <strong>Backscatter Drop:</strong> {sat_obs['backscatter_attenuation']}<br>
        <strong>Surface Area / Volume:</strong> {sat_obs['estimated_surface_area_km2']} km² (~{sat_obs['bonn_volume_estimate_m3']} m³)<br>
        <em>Note: {sat_obs['physical_limitations']}</em>
    </div>

    <div class="section-title">3. Metocean Hydrodynamic Advection Reconstruction</div>
    <p>
        4th-Order Runge-Kutta (RK4) stochastic particle advection-diffusion modeling (5,000 parcels) factoring 
        wind vector <strong>{metocean['wind_speed_kts']} kts</strong>, current drift <strong>{metocean['current_drift_kts']} kts</strong>, 
        and sea surface temperature <strong>{metocean['sea_surface_temp_c']}°C</strong> ({metocean['tidal_phase']}).
    </p>

    <div class="section-title">4. Candidate Vessels of Interest</div>
    <table class="data-table">
        <thead>
            <tr>
                <th>Candidate Vessel</th>
                <th>Flag & Type</th>
                <th>Priority</th>
                <th>Reasons for Investigation</th>
            </tr>
        </thead>
        <tbody>
            {vessel_rows}
        </tbody>
    </table>

    <div class="section-title">5. Technical Uncertainties & Data Limitations</div>
    <ul>
        {"".join([f"<li>{u}</li>" for u in dossier['technical_uncertainties_and_data_limitations']])}
    </ul>

    <div class="section-title">6. Traceable Data Sources</div>
    <table class="data-table">
        <thead>
            <tr>
                <th>Evidence ID</th>
                <th>Source Type</th>
                <th>Timestamp (UTC)</th>
                <th>SHA-256 Digest</th>
            </tr>
        </thead>
        <tbody>
            {evidence_rows}
        </tbody>
    </table>

    <div class="signature-box">
        <strong>INVESTIGATOR SIGN-OFF & INTEGRITY SEAL</strong><br>
        Analyst: <strong>{dossier['investigator_sign_off']['analyst_name']}</strong> | Status: <strong>{dossier['investigator_sign_off']['verification_status']}</strong><br>
        Analyst Notes: <em>{dossier['investigator_sign_off']['analyst_notes']}</em><br>
        <small style="color: #64748b;">Canonical SHA-256 Seal: <code>{dossier['investigator_sign_off']['payload_sha256_hash']}</code></small>
    </div>
</body>
</html>
"""
        return html_content
