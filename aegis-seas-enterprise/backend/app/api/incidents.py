"""
AEGIS-SEAS Enterprise: REST API Router for Incidents, Evidence, and Dossiers
"""

from fastapi import APIRouter, HTTPException, Query, Response
from typing import List, Dict, Any, Optional
from pydantic import BaseModel
from app.models.incident import CaseSchema, CaseStatus
from app.db import DB_CASES
from app.services.report_generator import InvestigationReportGenerator

router = APIRouter(prefix="/api", tags=["Incidents & Investigation Workspace"])

class StatusUpdatePayload(BaseModel):
    status: CaseStatus
    analyst_notes: Optional[str] = None

class AnalystNotePayload(BaseModel):
    analyst_notes: str

@router.get("/incidents", response_model=List[CaseSchema])
def list_incidents(
    status: Optional[CaseStatus] = None,
    search: Optional[str] = None
):
    results = list(DB_CASES.values())
    if status:
        results = [c for c in results if c.status == status]
    if search:
        s = search.lower()
        results = [c for c in results if s in c.incident_id.lower() or s in c.title.lower()]
    return results

@router.get("/incidents/{incident_id}", response_model=CaseSchema)
def get_incident_detail(incident_id: str):
    if incident_id not in DB_CASES:
        raise HTTPException(status_code=404, detail="Incident case not found")
    return DB_CASES[incident_id]

@router.post("/incidents/{incident_id}/status", response_model=CaseSchema)
def update_incident_status(incident_id: str, payload: StatusUpdatePayload):
    if incident_id not in DB_CASES:
        raise HTTPException(status_code=404, detail="Incident case not found")
    case = DB_CASES[incident_id]
    case.status = payload.status
    if payload.analyst_notes:
        case.analyst_notes = payload.analyst_notes
    return case

@router.post("/incidents/{incident_id}/notes", response_model=CaseSchema)
def add_analyst_notes(incident_id: str, payload: AnalystNotePayload):
    if incident_id not in DB_CASES:
        raise HTTPException(status_code=404, detail="Incident case not found")
    case = DB_CASES[incident_id]
    case.analyst_notes = payload.analyst_notes
    return case

@router.get("/incidents/{incident_id}/dossier/json")
def get_json_dossier(incident_id: str):
    if incident_id not in DB_CASES:
        raise HTTPException(status_code=404, detail="Incident case not found")
    case = DB_CASES[incident_id]
    return InvestigationReportGenerator.generate_json_dossier(case)

@router.get("/incidents/{incident_id}/dossier/html")
def get_html_dossier(incident_id: str):
    if incident_id not in DB_CASES:
        raise HTTPException(status_code=404, detail="Incident case not found")
    case = DB_CASES[incident_id]
    html_content = InvestigationReportGenerator.generate_printable_html_report(case)
    return Response(content=html_content, media_type="text/html")

@router.get("/vault")
def get_evidence_vault():
    all_evidence = []
    for case in DB_CASES.values():
        all_evidence.extend(case.evidence_vault)
    return all_evidence
