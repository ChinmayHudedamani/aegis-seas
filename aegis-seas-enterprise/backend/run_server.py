#!/usr/bin/env python3
"""
Server launcher for AEGIS-SEAS Enterprise Backend
Supports Uvicorn (FastAPI) and standalone Standard Library HTTP fallback daemon.
"""

import os
import sys
import json
import urllib.parse
from typing import Any
from http.server import SimpleHTTPRequestHandler, HTTPServer

sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))

from app.db import DB_CASES, seed_database
from app.services.report_generator import InvestigationReportGenerator
from app.models.incident import CaseStatus

class StandaloneBackendHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        seed_database()
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path

        if path in ("/", ""):
            self.send_json_response(200, {
                "platform": "AEGIS-SEAS Enterprise",
                "positioning": "Don't just detect the spill. Reconstruct the incident.",
                "version": "1.0.0-COMMERCIAL",
                "status": "OPERATIONAL"
            })
            return
        elif path in ("/api/incidents", "/api/incidents/"):
            cases = [c.dict() for c in DB_CASES.values()]
            self.send_json_response(200, cases)
            return
        elif path.startswith("/api/incidents/"):
            parts = path.strip("/").split("/")
            case_id = parts[2] if len(parts) >= 3 else ""

            if case_id not in DB_CASES:
                self.send_json_response(404, {"detail": "Incident case not found"})
                return

            case = DB_CASES[case_id]

            if len(parts) == 3:  # GET /api/incidents/{id}
                self.send_json_response(200, case.dict())
                return
            elif len(parts) >= 4 and parts[3] == "dossier":
                sub_format = parts[4] if len(parts) >= 5 else "json"
                if sub_format == "html":
                    html_content = InvestigationReportGenerator.generate_printable_html_report(case)
                    self.send_response(200)
                    self.send_header("Content-Type", "text/html")
                    self.end_headers()
                    self.wfile.write(html_content.encode("utf-8"))
                    return
                else:
                    json_dossier = InvestigationReportGenerator.generate_json_dossier(case)
                    self.send_json_response(200, json_dossier)
                    return

        elif path in ("/api/vault", "/api/vault/"):
            all_evidence = []
            for case in DB_CASES.values():
                all_evidence.extend([e.dict() for e in case.evidence_vault])
            self.send_json_response(200, all_evidence)
            return

        self.send_json_response(404, {"detail": "Endpoint not found"})

    def do_POST(self):
        seed_database()
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path

        content_length = int(self.headers.get('Content-Length', 0))
        body_json = {}
        if content_length > 0:
            try:
                body = self.rfile.read(content_length)
                body_json = json.loads(body.decode('utf-8'))
            except Exception:
                pass

        if path.startswith("/api/incidents/"):
            parts = path.strip("/").split("/")
            if len(parts) >= 4:
                case_id = parts[2]
                action = parts[3]

                if case_id in DB_CASES:
                    case = DB_CASES[case_id]
                    if action == "status":
                        new_status = body_json.get("status", case.status)
                        case.status = CaseStatus(new_status)
                        if "analyst_notes" in body_json:
                            case.analyst_notes = body_json["analyst_notes"]
                        self.send_json_response(200, case.dict())
                        return
                    elif action == "notes":
                        case.analyst_notes = body_json.get("analyst_notes", case.analyst_notes)
                        self.send_json_response(200, case.dict())
                        return

        self.send_json_response(400, {"detail": "Invalid POST request"})

    def send_json_response(self, code: int, data: Any):
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(data, indent=2).encode("utf-8"))

def main():
    seed_database()

    try:
        import uvicorn
        print("===================================================================")
        print("  AEGIS-SEAS ENTERPRISE: Commercial Incident Intelligence Server")
        print("  Positioning: 'Don't just detect the spill. Reconstruct the incident.'")
        print("  Engine     : Uvicorn / FastAPI Backend active on http://localhost:8000")
        print("===================================================================")
        uvicorn.run("app.main:app", host="0.0.0.0", port=8000, reload=True)
    except ImportError:
        port = 8000
        print("===================================================================")
        print("  AEGIS-SEAS ENTERPRISE: Commercial Incident Intelligence Server")
        print("  Positioning: 'Don't just detect the spill. Reconstruct the incident.'")
        print("  Engine     : Standalone HTTP Backend active on http://localhost:8000")
        print("===================================================================")
        server = HTTPServer(("", port), StandaloneBackendHandler)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down backend server.")
            server.server_close()

if __name__ == "__main__":
    main()
