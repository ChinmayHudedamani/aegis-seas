#!/usr/bin/env python3
"""
AEGIS-SEAS: Tactical Mission Control Local Web & API Server
Serves static assets from web/ with permissive CORS and exposes /api/run_pipeline
"""

import http.server
import socketserver
import subprocess
import json
import os
import sys
import mimetypes

PORT = 8080
ROOT_DIR = os.path.abspath(os.path.dirname(__file__))
WEB_DIR = os.path.join(ROOT_DIR, "web")

mimetypes.add_type("application/geo+json", ".geojson")
mimetypes.add_type("application/javascript", ".js")
mimetypes.add_type("text/css", ".css")

class TacticalRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def end_headers(self):
        # Permissive CORS headers for tactical GIS interoperability
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        if self.path in ("/api/run_pipeline", "/api/run_pipeline/"):
            self.handle_run_pipeline()
            return
        elif self.path in ("/api/status", "/api/status/"):
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            status_data = {
                "status": "ONLINE",
                "platform": "AEGIS-SEAS Sovereign C++20",
                "version": "1.0.0-SIH2026",
                "binary": os.path.exists(os.path.join(ROOT_DIR, "bin", "sar_oil_detector.exe"))
            }
            self.wfile.write(json.dumps(status_data).encode("utf-8"))
            return
        super().do_GET()

    def do_POST(self):
        if self.path in ("/api/run_pipeline", "/api/run_pipeline/"):
            self.handle_run_pipeline()
            return
        self.send_error(404, "Endpoint not found")

    def handle_run_pipeline(self):
        print("\n[Tactical Server] Executing C++20 AEGIS-SEAS Pipeline Orchestrator...")
        exe_path = os.path.join(ROOT_DIR, "bin", "sar_oil_detector.exe")
        
        try:
            cmd = f'cmd.exe /c "{exe_path}"'
            proc = subprocess.run(
                cmd,
                shell=True,
                cwd=ROOT_DIR,
                capture_output=True,
                text=True,
                timeout=30
            )

            stdout = proc.stdout
            stderr = proc.stderr
            return_code = proc.returncode

            print(f"[Tactical Server] Exit Code: {return_code}")
            if stdout:
                print(f"[Tactical Server Output]\n{stdout[:500]}...")

            geojson_path = os.path.join(WEB_DIR, "detected_spills.geojson")
            geojson_data = {}
            if os.path.exists(geojson_path):
                with open(geojson_path, "r", encoding="utf-8") as f:
                    geojson_data = json.load(f)

            response = {
                "success": return_code == 0,
                "exit_code": return_code,
                "stdout": stdout,
                "stderr": stderr,
                "features_count": len(geojson_data.get("features", [])),
                "geojson": geojson_data
            }

            self.send_response(200 if return_code == 0 else 500)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(response).encode("utf-8"))

        except Exception as ex:
            print(f"[Tactical Server] Error executing pipeline: {ex}", file=sys.stderr)
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            err_response = {"success": False, "error": str(ex)}
            self.wfile.write(json.dumps(err_response).encode("utf-8"))

def main():
    port = PORT
    if len(sys.argv) > 1:
        try:
            port = int(sys.argv[1])
        except ValueError:
            pass

    server = socketserver.TCPServer(("", port), TacticalRequestHandler)
    print(f"===================================================================")
    print(f"  AEGIS-SEAS Tactical Mission Control Server Active")
    print(f"  Local Dashboard URL : http://localhost:{port}")
    print(f"  API Pipeline Trigger : http://localhost:{port}/api/run_pipeline")
    print(f"  Serving Directory   : {WEB_DIR}")
    print(f"===================================================================")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[Tactical Server] Shutting down gracefully.")
        server.server_close()

if __name__ == "__main__":
    main()
