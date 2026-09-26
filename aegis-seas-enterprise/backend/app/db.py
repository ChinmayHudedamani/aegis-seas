"""
AEGIS-SEAS Enterprise Database Layer & Initial Seed Records
"""

from typing import List, Dict, Optional
from datetime import datetime
from app.models.incident import (
    CaseSchema, IncidentType, SeverityLevel, CaseStatus,
    InvestigationPriorityTier, EvidenceSourceType, CandidateVessel,
    EvidenceVaultItem, EstimatedTimeWindow, ProbableOriginZone, EnvironmentalContext
)

# In-Memory Database Store for Enterprise Workspace Cases
DB_CASES: Dict[str, CaseSchema] = {}

def seed_database():
    if DB_CASES:
        return

    # Seed Case 1: Mumbai High Offshore Anomaly
    case1 = CaseSchema(
        incident_id="AEGIS-00124",
        title="Offshore Sector 4 Unidentified Hydrocarbon Anomaly",
        coordinates=(19.3347, 71.3662),
        recorded_at="2026-09-24T14:30:00Z",
        incident_type=IncidentType.MINERAL_OIL_DISCHARGE,
        severity=SeverityLevel.HIGH,
        status=CaseStatus.IN_REVIEW,
        assigned_investigator="Lead Analyst M. Henderson",
        estimated_window=EstimatedTimeWindow(
            start_time="2026-09-24T10:00:00Z",
            end_time="2026-09-24T12:30:00Z",
            confidence_pct=94.5
        ),
        probable_origin_zone=ProbableOriginZone(
            type="Polygon",
            coordinates=[[
                [71.3500, 19.3200], [71.3800, 19.3200],
                [71.3850, 19.3500], [71.3450, 19.3500],
                [71.3500, 19.3200]
            ]]
        ),
        environmental_context=EnvironmentalContext(
            wind_speed_kts=13.2,
            sea_surface_temp_c=28.5,
            tidal_phase="Ebb Tide (SW Drift)",
            current_drift_kts=0.47
        ),
        candidate_vessels=[
            CandidateVessel(
                vessel_id="IMO-9428812 / MMSI-636019842",
                vessel_name="NEPTUNE_TRANSIT",
                flag_state="Panama",
                vessel_type="Crude Oil Tanker",
                investigation_priority=InvestigationPriorityTier.TIER_1,
                why_this_vessel=[
                    "Sailed through the probable origin region during the estimated discharge window.",
                    "Speed reduction from 15.2 kts cruising to 7.1 kts (typical auxiliary discharge range).",
                    "Radon wake kinematics identify trajectory collinearity with slick axis."
                ],
                metrics={
                    "distance_to_centroid_nm": 0.78,
                    "temporal_offset_minutes": 14,
                    "speed_knots": 7.1,
                    "heading_deg": 142.0
                }
            ),
            CandidateVessel(
                vessel_id="IMO-9102441 / MMSI-412888901",
                vessel_name="SHADOW_CARRIER",
                flag_state="Liberia",
                vessel_type="Product Tanker",
                investigation_priority=InvestigationPriorityTier.TIER_1,
                why_this_vessel=[
                    "Encountered 42-minute Class-A AIS reception gap across the origin boundary.",
                    "Radar hard target verified by 2D CA-CFAR coincident with historical dead-reckoned corridor."
                ],
                metrics={
                    "distance_to_centroid_nm": 1.25,
                    "temporal_offset_minutes": 38,
                    "speed_knots": 12.4,
                    "heading_deg": 138.5
                }
            ),
            CandidateVessel(
                vessel_id="IMO-9812450 / MMSI-538009115",
                vessel_name="FAST_RUNNER",
                flag_state="Marshall Islands",
                vessel_type="Container Ship",
                investigation_priority=InvestigationPriorityTier.TIER_3,
                why_this_vessel=[
                    "Transit track verified on commercial shipping lane 4.2 nm north of origin region."
                ],
                metrics={
                    "distance_to_centroid_nm": 4.20,
                    "temporal_offset_minutes": 85,
                    "speed_knots": 21.0,
                    "heading_deg": 260.0
                }
            )
        ],
        evidence_vault=[
            EvidenceVaultItem(
                evidence_id="EV-SAR-001",
                incident_id="AEGIS-00124",
                source_type=EvidenceSourceType.SATELLITE_SAR,
                timestamp_utc="2026-09-24T14:30:00Z",
                metadata_proof={
                    "sensor_id": "Sentinel-1 C-Band SAR",
                    "orbit_pass": "Relative Orbit 129 / IW Swath",
                    "ingest_checksum_sha256": "a8f5f167f44f4964e6c998dee827110c",
                    "source_provider": "Copernicus Open Access Hub"
                },
                file_url="/data/rasters/S1A_IW_GRDH_1SDV_20260924T143000.tif"
            ),
            EvidenceVaultItem(
                evidence_id="EV-AIS-004",
                incident_id="AEGIS-00124",
                source_type=EvidenceSourceType.SATELLITE_AIS,
                timestamp_utc="2026-09-24T14:32:10Z",
                metadata_proof={
                    "sensor_id": "Spire Global Satellite AIS",
                    "orbit_pass": "Constellation Pass 841",
                    "ingest_checksum_sha256": "7c9e12b489a2410ef4129b00881928cf",
                    "source_provider": "Spire Maritime Feed"
                },
                file_url="/data/ais/ais_track_AEGIS-00124.csv"
            ),
            EvidenceVaultItem(
                evidence_id="EV-MET-002",
                incident_id="AEGIS-00124",
                source_type=EvidenceSourceType.INCOIS_ROMS,
                timestamp_utc="2026-09-24T12:00:00Z",
                metadata_proof={
                    "sensor_id": "INCOIS ROMS High-Res Ocean Current Model",
                    "orbit_pass": "Grid 0.05deg Resolution",
                    "ingest_checksum_sha256": "12f8491a92e104b2c129088fa12847a9",
                    "source_provider": "Indian National Centre for Ocean Information Services"
                },
                file_url="/data/metocean/roms_currents_20260924.nc"
            )
        ],
        analyst_notes="Initial SAR imagery ingested. Marangoni attenuation contrast exceeds 8.9 dB. Candidates NEPTUNE_TRANSIT and SHADOW_CARRIER flagged for high temporal-spatial correlation.",
        created_at="2026-09-24T14:35:00Z",
        updated_at="2026-09-24T15:10:00Z"
    )

    # Seed Case 2: Gulf of Kutch Refinery Approach
    case2 = CaseSchema(
        incident_id="AEGIS-00125",
        title="Gulf of Kutch SPM Terminal Surface Sheen Anomaly",
        coordinates=(22.4500, 69.3500),
        recorded_at="2026-09-25T08:15:00Z",
        incident_type=IncidentType.UNKNOWN_SHEEN,
        severity=SeverityLevel.MEDIUM,
        status=CaseStatus.ACTIVE_ALERT,
        assigned_investigator="Senior Investigator R. Vance",
        estimated_window=EstimatedTimeWindow(
            start_time="2026-09-25T04:30:00Z",
            end_time="2026-09-25T07:00:00Z",
            confidence_pct=91.2
        ),
        probable_origin_zone=ProbableOriginZone(
            type="Polygon",
            coordinates=[[
                [69.3300, 22.4400], [69.3700, 22.4400],
                [69.3750, 22.4600], [69.3250, 22.4600],
                [69.3300, 22.4400]
            ]]
        ),
        environmental_context=EnvironmentalContext(
            wind_speed_kts=14.5,
            sea_surface_temp_c=27.2,
            tidal_phase="Flood Tide",
            current_drift_kts=0.58
        ),
        candidate_vessels=[
            CandidateVessel(
                vessel_id="IMO-9781249 / MMSI-419001248",
                vessel_name="INDIAN_GLORY",
                flag_state="India",
                vessel_type="Very Large Crude Carrier (VLCC)",
                investigation_priority=InvestigationPriorityTier.TIER_2,
                why_this_vessel=[
                    "Maneuvered at Single Point Mooring terminal approach during estimated window."
                ],
                metrics={
                    "distance_to_centroid_nm": 1.10,
                    "temporal_offset_minutes": 22,
                    "speed_knots": 3.2,
                    "heading_deg": 90.0
                }
            )
        ],
        evidence_vault=[
            EvidenceVaultItem(
                evidence_id="EV-SAR-002",
                incident_id="AEGIS-00125",
                source_type=EvidenceSourceType.SATELLITE_SAR,
                timestamp_utc="2026-09-25T08:15:00Z",
                metadata_proof={
                    "sensor_id": "Sentinel-1 C-Band SAR",
                    "orbit_pass": "Relative Orbit 027 / IW Swath",
                    "ingest_checksum_sha256": "4b8e912409f8721c128490a12908f01b",
                    "source_provider": "Copernicus Open Access Hub"
                },
                file_url="/data/rasters/S1A_KUTCH_20260925T081500.tif"
            )
        ],
        analyst_notes="Alert flagged by automated coastal sweep. Verification in progress.",
        created_at="2026-09-25T08:20:00Z",
        updated_at="2026-09-25T08:20:00Z"
    )

    DB_CASES[case1.incident_id] = case1
    DB_CASES[case2.incident_id] = case2

seed_database()
