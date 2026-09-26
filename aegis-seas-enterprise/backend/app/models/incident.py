"""
AEGIS-SEAS Enterprise: Relational Case-Management Data Schemas
Strict Legal Terminology Conformance & Lightweight Fallback Compatibility Layer
"""

from enum import Enum
from typing import List, Dict, Any, Optional, Tuple
from datetime import datetime

try:
    from pydantic import BaseModel, Field
except ImportError:
    # Standard library fallback if Pydantic is not installed
    class BaseModel:
        def __init__(self, **kwargs):
            for k, v in kwargs.items():
                setattr(self, k, v)
        def dict(self):
            res = {}
            for k, v in self.__dict__.items():
                if isinstance(v, Enum):
                    res[k] = v.value
                elif isinstance(v, list):
                    res[k] = [item.dict() if hasattr(item, 'dict') else (item.value if isinstance(item, Enum) else item) for item in v]
                elif hasattr(v, 'dict'):
                    res[k] = v.dict()
                else:
                    res[k] = v
            return res

    def Field(default=..., description=""):
        return default

class IncidentType(str, Enum):
    MINERAL_OIL_DISCHARGE = "Mineral Oil / Bilge Discharge"
    UNKNOWN_SHEEN = "Unknown Sheen"
    BUNKER_LEAK = "Bunker Leak"

class SeverityLevel(str, Enum):
    LOW = "Low"
    MEDIUM = "Medium"
    HIGH = "High"
    CRITICAL = "Critical"

class CaseStatus(str, Enum):
    ACTIVE_ALERT = "Active Alert"
    IN_REVIEW = "In Review"
    CONFIRMED_DISCHARGE = "Confirmed Discharge"
    REJECTED_FALSE_POSITIVE = "Rejected / False Positive"
    ARCHIVED_CASE = "Archived Case"

class InvestigationPriorityTier(str, Enum):
    TIER_1 = "Tier 1 - Immediate Investigation"
    TIER_2 = "Tier 2 - Secondary Review"
    TIER_3 = "Tier 3 - Low Correlation"

class EvidenceSourceType(str, Enum):
    SATELLITE_SAR = "Satellite SAR"
    OPTICAL_EO = "Optical EO"
    TERRESTRIAL_AIS = "Terrestrial AIS"
    SATELLITE_AIS = "Satellite AIS"
    INCOIS_ROMS = "INCOIS ROMS"
    ANALYST_NOTE = "Analyst Note"

class CandidateVessel(BaseModel):
    vessel_id: str
    vessel_name: str
    flag_state: str
    vessel_type: str
    investigation_priority: InvestigationPriorityTier
    why_this_vessel: List[str]
    metrics: Dict[str, Any]

class EvidenceVaultItem(BaseModel):
    evidence_id: str
    incident_id: str
    source_type: EvidenceSourceType
    timestamp_utc: str
    metadata_proof: Dict[str, Any]
    file_url: str

class EstimatedTimeWindow(BaseModel):
    start_time: str
    end_time: str
    confidence_pct: float

class ProbableOriginZone(BaseModel):
    type: str = "Polygon"
    coordinates: List[List[List[float]]]

class EnvironmentalContext(BaseModel):
    wind_speed_kts: float
    sea_surface_temp_c: float
    tidal_phase: str
    current_drift_kts: float

class CaseSchema(BaseModel):
    incident_id: str
    title: str
    coordinates: Tuple[float, float]
    recorded_at: str
    incident_type: IncidentType
    severity: SeverityLevel
    status: CaseStatus
    assigned_investigator: str
    estimated_window: EstimatedTimeWindow
    probable_origin_zone: ProbableOriginZone
    environmental_context: EnvironmentalContext
    candidate_vessels: List[CandidateVessel] = []
    evidence_vault: List[EvidenceVaultItem] = []
    analyst_notes: Optional[str] = None
    created_at: str
    updated_at: str
