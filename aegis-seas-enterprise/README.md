# AEGIS-SEAS ENTERPRISE 🌊🏢
### Commercial Maritime Incident Intelligence & Reconstruction Platform

> **Core Positioning**: *"Don't just detect the spill. Reconstruct the incident."*

**AEGIS-SEAS Enterprise** is a high-stakes, audit-ready maritime investigation workspace built for Maritime Authorities, P&I Insurance Clubs, Harbor Police, and Statutory Compliance Teams.

---

## 🏛️ System Architecture & Repository Separation

```text
├── aegis-seas-core/               <-- DEFENSE-GRADE BARE-METAL C++20 ENGINE
│   ├── src/engine_sar/            (Zero-copy mmap, O(1) 2D CA-CFAR, Lee speckle)
│   ├── src/engine_drift/          (5,000-particle stochastic RK4 solver)
│   ├── src/engine_kinematics/     (Radon wake inversion, Doppler SAR speed)
│   └── src/engine_dossier/        (Bonn volumetrics, MARPOL Reg 15, ECDSA)
│
└── aegis-seas-enterprise/         <-- COMMERCIAL INVESTIGATION WORKSPACE
    ├── backend/                   (FastAPI case management, REST endpoints, PDF dossier engine)
    ├── frontend/                  (Next.js / TypeScript / Tailwind tactical multi-pane workspace)
    └── docs/                      (P&I Club pitch deck & commercial competitive battlecard)
```

---

## 🚀 Quickstart: Running Enterprise Backend & Workspace

### 1. Launch FastAPI Backend
```bash
cd aegis-seas-enterprise/backend
python run_server.py
```
* **API Documentation**: [`http://localhost:8000/docs`](http://localhost:8000/docs)
* **Sample Case Endpoint**: [`http://localhost:8000/api/incidents/AEGIS-00124`](http://localhost:8000/api/incidents/AEGIS-00124)
* **Printable Dossier Endpoint**: [`http://localhost:8000/api/incidents/AEGIS-00124/dossier/html`](http://localhost:8000/api/incidents/AEGIS-00124/dossier/html)

### 2. Launch Enterprise Frontend Workspace
```bash
cd aegis-seas-enterprise/frontend
npm run dev
```
Open **`http://localhost:3000`** in your browser.

---

## ⚖️ Strict Terminology Compliance Standards

1. **Candidate Vessels**: Always uses *"Candidate Vessel"*, *"Potential Source Vessel"*, or *"Investigation Priority"*.
2. **Investigation Targets**: Always uses *"Identified vessels warranting further investigation"*.
3. **Spatial Origin**: Always uses *"Probable origin region and estimated time window"*.
4. **Legal Evidence**: Always uses *"Investigation-ready, traceable evidence dossier"*.
5. **Sensor Limits**: Always uses *"All-weather SAR radar capability with physical sea-state limitations"*.

---

## 📜 License
Commercial Enterprise License. Proprietary to AEGIS-SEAS Technologies.
