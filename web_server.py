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
import urllib.parse

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
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        if path in ("/api/run_pipeline", "/api/run_pipeline/"):
            self.handle_run_pipeline(query)
            return
        elif path in ("/api/status", "/api/status/"):
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            status_data = {
                "status": "ONLINE",
                "platform": "AEGIS-SEAS Sovereign C++20",
                "theater": "Indian Ocean & Arabian Sea EEZ",
                "version": "1.0.0-SIH2026",
                "binary": os.path.exists(os.path.join(ROOT_DIR, "bin", "sar_oil_detector.exe")),
                "weights": os.path.exists(os.path.join(ROOT_DIR, "data", "unet_weights.bin"))
            }
            self.wfile.write(json.dumps(status_data).encode("utf-8"))
            return
        elif path in ("/api/sectors", "/api/sectors/"):
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            sectors = {
                "mumbai_high": {
                    "id": "mumbai_high",
                    "name": "Mumbai High Offshore Basin (Arabian Sea)",
                    "description": "ONGC offshore drilling platforms & Western tanker fairway",
                    "center": [19.33, 71.37],
                    "bounds": [[19.20, 71.20], [19.50, 71.55]],
                    "wind_speed": 6.8,
                    "wind_dir": 240.0,
                    "current_u": 0.22,
                    "current_v": -0.08
                },
                "gulf_of_kutch": {
                    "id": "gulf_of_kutch",
                    "name": "Gulf of Kutch / Jamnagar SPM Terminal (Gujarat)",
                    "description": "World's largest refining complex, VLCC single point mooring approach",
                    "center": [22.45, 69.35],
                    "bounds": [[22.30, 69.15], [22.60, 69.55]],
                    "wind_speed": 7.5,
                    "wind_dir": 250.0,
                    "current_u": 0.30,
                    "current_v": 0.10
                },
                "southern_corridor": {
                    "id": "southern_corridor",
                    "name": "Southern Shipping Highway (Sri Lanka - Nicobar)",
                    "description": "High-density East-West crude tanker corridor & Malacca approach",
                    "center": [5.95, 80.55],
                    "bounds": [[5.75, 80.35], [6.15, 80.75]],
                    "wind_speed": 8.2,
                    "wind_dir": 225.0,
                    "current_u": 0.38,
                    "current_v": -0.05
                }
            }
            self.wfile.write(json.dumps(sectors).encode("utf-8"))
            return
        elif path in ("/api/model_metrics", "/api/model_metrics/"):
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            metrics_path = os.path.join(ROOT_DIR, "data", "model_metrics.json")
            metrics = {}
            if os.path.exists(metrics_path):
                with open(metrics_path, "r", encoding="utf-8") as f:
                    metrics = json.load(f)
            self.wfile.write(json.dumps(metrics).encode("utf-8"))
            return
        super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        if path in ("/api/run_pipeline", "/api/run_pipeline/"):
            # Check if JSON body provided
            content_length = int(self.headers.get('Content-Length', 0))
            body_json = {}
            if content_length > 0:
                try:
                    body = self.rfile.read(content_length)
                    body_json = json.loads(body.decode('utf-8'))
                except Exception:
                    pass
            sector = body_json.get("sector", query.get("sector", ["mumbai_high"])[0])
            self.handle_run_pipeline({"sector": [sector]})
            return
        elif path in ("/api/train_model", "/api/train_model/"):
            self.handle_train_model()
            return
        self.send_error(404, "Endpoint not found")

    def handle_train_model(self):
        print("\n[Tactical Server] Triggering U-Net Training on Real Sentinel-1 SAR Dataset...")
        try:
            train_script = os.path.join(ROOT_DIR, "tools", "train_unet.py")
            cmd = f'python "{train_script}" 10'
            proc = subprocess.run(
                cmd,
                shell=True,
                cwd=ROOT_DIR,
                capture_output=True,
                text=True,
                timeout=120
            )
            metrics_path = os.path.join(ROOT_DIR, "data", "model_metrics.json")
            metrics = {}
            if os.path.exists(metrics_path):
                with open(metrics_path, "r", encoding="utf-8") as f:
                    metrics = json.load(f)

            self.send_response(200 if proc.returncode == 0 else 500)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            resp = {
                "success": proc.returncode == 0,
                "stdout": proc.stdout,
                "stderr": proc.stderr,
                "metrics": metrics
            }
            self.wfile.write(json.dumps(resp).encode("utf-8"))
        except Exception as ex:
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"success": False, "error": str(ex)}).encode("utf-8"))

    def handle_run_pipeline(self, query=None):
        sector = "mumbai_high"
        if query and "sector" in query and query["sector"]:
            sector = query["sector"][0]

        print(f"\n[Tactical Server] Executing C++20 AEGIS-SEAS Pipeline Orchestrator for sector: {sector}...")
        exe_path = os.path.join(ROOT_DIR, "bin", "sar_oil_detector.exe")
        ais_path = os.path.join(ROOT_DIR, "data", "indian_ocean_ais_traffic.csv")
        weights_path = os.path.join(ROOT_DIR, "data", "unet_weights.bin")
        
        try:
            args = [exe_path, "--sector", sector, "--ais", ais_path, "--weights", weights_path]
            proc = subprocess.run(
                args,
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
                "sector": sector,
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

    socketserver.TCPServer.allow_reuse_address = True
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
