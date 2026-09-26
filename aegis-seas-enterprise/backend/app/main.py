"""
AEGIS-SEAS Enterprise: FastAPI Core Application Entrypoint
Commercial Maritime Incident Intelligence & Reconstruction Platform
"""

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.api.incidents import router as incidents_router
from app.db import seed_database

app = FastAPI(
    title="AEGIS-SEAS Enterprise API",
    description="Commercial Maritime Incident Intelligence & Reconstruction Platform",
    version="1.0.0"
)

# Configure CORS for Enterprise Web Workspace
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.on_event("startup")
def on_startup():
    seed_database()

app.include_router(incidents_router)

@app.get("/")
def root():
    return {
        "platform": "AEGIS-SEAS Enterprise",
        "positioning": "Don't just detect the spill. Reconstruct the incident.",
        "version": "1.0.0-COMMERCIAL",
        "docs_url": "/docs"
    }
