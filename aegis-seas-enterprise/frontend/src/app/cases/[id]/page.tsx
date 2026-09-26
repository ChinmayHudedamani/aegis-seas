"use client";

import { useState } from 'react';
import Link from 'next/link';

export default function IncidentWorkspacePage({ params }: { params: { id: string } }) {
  const caseId = params.id || 'AEGIS-00124';

  const [status, setStatus] = useState<string>('In Review');
  const [analystNotes, setAnalystNotes] = useState<string>(
    'Initial SAR imagery ingested. Marangoni attenuation contrast exceeds 8.9 dB. Candidates NEPTUNE_TRANSIT and SHADOW_CARRIER flagged for high temporal-spatial correlation.'
  );
  const [showReportModal, setShowReportModal] = useState<boolean>(false);

  const candidateVessels = [
    {
      id: 'IMO-9428812 / MMSI-636019842',
      name: 'NEPTUNE_TRANSIT',
      flag: 'Panama',
      type: 'Crude Oil Tanker',
      tier: 'Tier 1 - Immediate Investigation',
      reasons: [
        'Sailed through the probable origin region during the estimated discharge window.',
        'Speed reduction from 15.2 kts cruising to 7.1 kts (typical auxiliary discharge range).',
        'Radon wake kinematics identify trajectory collinearity with slick axis.'
      ],
      metrics: { dist: '0.78 nm', offset: '14 min', speed: '7.1 kts', heading: '142°' }
    },
    {
      id: 'IMO-9102441 / MMSI-412888901',
      name: 'SHADOW_CARRIER',
      flag: 'Liberia',
      type: 'Product Tanker',
      tier: 'Tier 1 - Immediate Investigation',
      reasons: [
        'Encountered 42-minute Class-A AIS reception gap across the origin boundary.',
        'Radar hard target verified by 2D CA-CFAR coincident with historical dead-reckoned corridor.'
      ],
      metrics: { dist: '1.25 nm', offset: '38 min', speed: '12.4 kts', heading: '138°' }
    },
    {
      id: 'IMO-9812450 / MMSI-538009115',
      name: 'FAST_RUNNER',
      flag: 'Marshall Islands',
      type: 'Container Ship',
      tier: 'Tier 3 - Low Correlation',
      reasons: [
        'Transit track verified on commercial shipping lane 4.2 nm north of origin region.'
      ],
      metrics: { dist: '4.20 nm', offset: '85 min', speed: '21.0 kts', heading: '260°' }
    }
  ];

  const timelineEvents = [
    { time: '10:00 UTC', title: 'Estimated Discharge Window Opens', desc: 'Probable origin region active.', type: 'window' },
    { time: '11:15 UTC', title: 'AIS Blackout Identified', desc: 'SHADOW_CARRIER transponder silent for 42 mins.', type: 'ais' },
    { time: '11:42 UTC', title: 'Speed Drop Recorded', desc: 'NEPTUNE_TRANSIT reduced speed to 7.1 kts.', type: 'vessel' },
    { time: '12:30 UTC', title: 'Estimated Discharge Window Closes', desc: 'Slick advection phase begins.', type: 'window' },
    { time: '14:30 UTC', title: 'Sentinel-1 SAR Pass Ingested', desc: 'C-Band radar pass captured 2.508 km² slick.', type: 'sar' },
  ];

  return (
    <main className="flex-1 flex flex-col bg-slate-950 overflow-hidden">
      {/* Top Header Bar */}
      <div className="bg-slate-900 border-b border-slate-800 px-6 py-3 flex flex-wrap justify-between items-center gap-4">
        <div>
          <div className="flex items-center gap-3">
            <span className="font-mono font-extrabold text-cyan-400 text-sm">{caseId}</span>
            <h1 className="font-extrabold text-white text-base">
              Offshore Sector 4 Unidentified Hydrocarbon Anomaly
            </h1>
            <span className="px-2.5 py-0.5 rounded text-xs font-bold bg-yellow-950 text-yellow-400 border border-yellow-800">
              {status}
            </span>
          </div>
          <div className="text-xs text-slate-400 mt-0.5">
            Centroid: <strong>19.3347°N, 71.3662°E</strong> (Mumbai High Basin) | Recorded: 2026-09-24 14:30 UTC
          </div>
        </div>

        <div className="flex items-center gap-3">
          <button
            onClick={() => setShowReportModal(true)}
            className="bg-cyan-600 hover:bg-cyan-500 text-white font-bold text-xs px-4 py-2 rounded shadow transition-colors flex items-center gap-2"
          >
            <span>📜</span>
            <span>GENERATE INVESTIGATION REPORT</span>
          </button>
        </div>
      </div>

      {/* Three-Column Operational Layout */}
      <div className="flex-1 grid grid-cols-1 lg:grid-cols-12 gap-0 overflow-hidden">
        {/* LEFT COLUMN: Case Metadata & Evidence Timeline (3 cols) */}
        <section className="lg:col-span-3 bg-slate-900/90 border-r border-slate-800 p-4 overflow-y-auto space-y-4 flex flex-col">
          <div className="border-b border-slate-800 pb-3">
            <h2 className="font-extrabold text-xs text-slate-200 uppercase tracking-wider flex items-center gap-2">
              <span>⏱️</span> Chronological Incident Reconstruction
            </h2>
            <p className="text-[11px] text-slate-400 mt-1">
              What happened and when? Probable origin region & estimated time window.
            </p>
          </div>

          {/* Environmental Context Card */}
          <div className="bg-slate-950 border border-slate-800 p-3 rounded-lg text-xs space-y-1.5">
            <div className="font-bold text-slate-300 text-[11px] uppercase">Environmental Context</div>
            <div className="grid grid-cols-2 gap-2 text-[11px] text-slate-400">
              <div>Wind: <strong className="text-slate-200">13.2 kts</strong></div>
              <div>Current: <strong className="text-slate-200">0.47 kts</strong></div>
              <div>SST: <strong className="text-slate-200">28.5 °C</strong></div>
              <div>Tide: <strong className="text-slate-200">SW Ebb</strong></div>
            </div>
          </div>

          {/* Timeline */}
          <div className="space-y-3 flex-1">
            <div className="font-bold text-slate-300 text-[11px] uppercase">Reconstruction Timeline</div>
            <div className="space-y-3 relative before:absolute before:left-2 before:top-2 before:bottom-2 before:w-0.5 before:bg-slate-800">
              {timelineEvents.map((evt, idx) => (
                <div key={idx} className="relative pl-6 text-xs">
                  <div className="absolute left-0 top-1 w-4 h-4 rounded-full bg-slate-900 border-2 border-cyan-500 flex items-center justify-center"></div>
                  <div className="font-mono text-[10px] text-cyan-400 font-bold">{evt.time}</div>
                  <div className="font-bold text-slate-200 text-[11px]">{evt.title}</div>
                  <div className="text-[11px] text-slate-400 leading-snug">{evt.desc}</div>
                </div>
              ))}
            </div>
          </div>

          {/* Physical Limitations Caveat */}
          <div className="bg-slate-950/60 border border-slate-800/80 p-2.5 rounded text-[10px] text-slate-400 leading-tight">
            <strong className="text-slate-300">Technical Capability Statement:</strong> All-weather SAR radar capability with physical sea-state limitations.
          </div>
        </section>

        {/* CENTER COLUMN: Interactive Investigation Map (6 cols) */}
        <section className="lg:col-span-6 bg-slate-950 flex flex-col relative border-r border-slate-800">
          <div className="bg-slate-900/80 border-b border-slate-800 px-4 py-2 text-xs flex justify-between items-center">
            <span className="font-bold text-slate-300 flex items-center gap-2">
              <span>🌐</span> Tactical Investigation Radar Scene (ESRI Dark Canvas)
            </span>
            <span className="text-[11px] text-cyan-400 font-mono">95% Spatial Confidence Origin Ellipse Active</span>
          </div>

          {/* Map Container Mock Canvas */}
          <div className="flex-1 bg-slate-950 relative flex items-center justify-center p-6 overflow-hidden">
            {/* Visual Radar Mock Layers */}
            <div className="absolute inset-0 bg-[radial-gradient(#1e293b_1px,transparent_1px)] [background-size:16px_16px] opacity-40"></div>

            {/* Tactical Anomaly Overlay */}
            <div className="relative z-10 text-center space-y-4">
              <div className="inline-block border-2 border-red-500 bg-red-950/40 p-6 rounded-2xl shadow-2xl backdrop-blur-sm">
                <div className="text-xs font-mono text-red-400 font-bold">DETECTED HYDROCARBON ANOMALY POLYGON</div>
                <div className="text-sm font-extrabold text-white mt-1">Area: 2.508 km² | Bonn Volume: ~45.0 m³</div>
                <div className="flex justify-center gap-2 mt-3 text-[10px] font-bold">
                  <span className="px-2 py-0.5 rounded bg-sky-950 text-sky-400 border border-sky-800">Sheen (0.08µm)</span>
                  <span className="px-2 py-0.5 rounded bg-cyan-950 text-cyan-400 border border-cyan-800">Rainbow (2.5µm)</span>
                  <span className="px-2 py-0.5 rounded bg-yellow-950 text-yellow-400 border border-yellow-800">True Oil (25µm)</span>
                  <span className="px-2 py-0.5 rounded bg-red-950 text-red-400 border border-red-800">Emulsion (200µm)</span>
                </div>
              </div>

              {/* 95% Origin Ellipse */}
              <div className="border border-dashed border-cyan-400 bg-cyan-950/20 p-4 rounded-full max-w-md mx-auto">
                <div className="text-[11px] font-mono text-cyan-300 font-bold">
                  95% Stochastic Spatial Confidence Origin Ellipse (3.73 km²)
                </div>
                <div className="text-[10px] text-slate-400">RK4 Backward Hydrodynamic Advection (-4.0h Hindcast)</div>
              </div>

              {/* Candidate Trajectory Line Mock */}
              <div className="text-xs font-mono text-slate-400 bg-slate-900/90 border border-slate-800 p-2.5 rounded-lg inline-block">
                🚢 NEPTUNE_TRANSIT Track: <span className="text-cyan-400">15.2 kts &rarr; 7.1 kts Speed Reduction</span> at Origin Boundary
              </div>
            </div>
          </div>
        </section>

        {/* RIGHT COLUMN: "Why This Vessel?" & Human-in-the-Loop Review (3 cols) */}
        <section className="lg:col-span-3 bg-slate-900/90 p-4 overflow-y-auto space-y-4 flex flex-col">
          <div className="border-b border-slate-800 pb-3">
            <h2 className="font-extrabold text-xs text-slate-200 uppercase tracking-wider flex items-center gap-2">
              <span>🚢</span> Candidate Source Vessels
            </h2>
            <p className="text-[11px] text-slate-400 mt-1">
              Identified vessels warranting further investigation (Ranked by spatial-temporal correlation).
            </p>
          </div>

          {/* Ranked Candidates */}
          <div className="space-y-3 flex-1">
            {candidateVessels.map((v, idx) => (
              <div key={idx} className="bg-slate-950 border border-slate-800 rounded-lg p-3 space-y-2 text-xs">
                <div className="flex justify-between items-start">
                  <div>
                    <div className="font-bold text-slate-100">{v.name}</div>
                    <div className="text-[10px] text-slate-500 font-mono">{v.id}</div>
                  </div>
                  <span className={`px-2 py-0.5 rounded text-[9px] font-bold ${
                    v.tier.includes('Tier 1') ? 'bg-red-950 text-red-400 border border-red-800' : 'bg-slate-800 text-slate-400'
                  }`}>
                    {v.tier}
                  </span>
                </div>

                <div className="text-[11px] text-slate-400">
                  Flag: <strong className="text-slate-200">{v.flag}</strong> | Type: <strong className="text-slate-200">{v.type}</strong>
                </div>

                <div className="bg-slate-900/80 p-2 rounded text-[11px] space-y-1">
                  <div className="font-bold text-cyan-400 text-[10px] uppercase">Forensic Rationale:</div>
                  <ul className="list-disc pl-3 text-slate-300 space-y-0.5 leading-snug">
                    {v.reasons.map((r, rIdx) => (
                      <li key={rIdx}>{r}</li>
                    ))}
                  </ul>
                </div>
              </div>
            ))}
          </div>

          {/* Human-in-the-Loop Review Panel */}
          <div className="bg-slate-950 border border-slate-800 rounded-lg p-3 space-y-3 text-xs">
            <div className="font-bold text-slate-200 text-[11px] uppercase">Human-in-the-Loop Review Panel</div>

            <div className="grid grid-cols-3 gap-1.5">
              <button
                onClick={() => setStatus('Confirmed Discharge')}
                className={`py-1.5 rounded font-bold text-[10px] transition-colors ${
                  status === 'Confirmed Discharge' ? 'bg-red-600 text-white' : 'bg-slate-800 text-slate-300 hover:bg-red-900'
                }`}
              >
                Confirm Discharge
              </button>
              <button
                onClick={() => setStatus('In Review')}
                className={`py-1.5 rounded font-bold text-[10px] transition-colors ${
                  status === 'In Review' ? 'bg-yellow-600 text-white' : 'bg-slate-800 text-slate-300 hover:bg-yellow-900'
                }`}
              >
                In Review
              </button>
              <button
                onClick={() => setStatus('Rejected / False Positive')}
                className={`py-1.5 rounded font-bold text-[10px] transition-colors ${
                  status === 'Rejected / False Positive' ? 'bg-slate-700 text-white' : 'bg-slate-800 text-slate-300 hover:bg-slate-700'
                }`}
              >
                Reject Anomaly
              </button>
            </div>

            <div>
              <label className="block text-[10px] font-bold text-slate-400 uppercase mb-1">
                Mandatory Investigator Case Notes:
              </label>
              <textarea
                value={analystNotes}
                onChange={(e) => setAnalystNotes(e.target.value)}
                rows={3}
                className="w-full bg-slate-900 border border-slate-800 rounded p-2 text-xs text-slate-200 focus:outline-none focus:border-cyan-500"
              />
            </div>

            <button
              onClick={() => setShowReportModal(true)}
              className="w-full bg-cyan-600 hover:bg-cyan-500 text-white font-bold text-xs py-2 rounded transition-colors shadow"
            >
              Generate Investigation Report &rarr;
            </button>
          </div>
        </section>
      </div>

      {/* Modal: Generated Investigation Report */}
      {showReportModal && (
        <div className="fixed inset-0 z-50 bg-black/80 flex items-center justify-center p-4 backdrop-blur-sm">
          <div className="bg-slate-900 border border-slate-800 rounded-xl max-w-3xl w-full max-h-[90vh] flex flex-col shadow-2xl">
            <div className="p-4 border-b border-slate-800 flex justify-between items-center">
              <div>
                <h3 className="font-extrabold text-white text-base">AEGIS-SEAS Enterprise Investigation Dossier</h3>
                <p className="text-xs text-cyan-400 font-mono">Case ID: {caseId} | Status: {status}</p>
              </div>
              <button
                onClick={() => setShowReportModal(false)}
                className="text-slate-400 hover:text-white font-bold text-xl"
              >
                &times;
              </button>
            </div>

            <div className="p-6 overflow-y-auto space-y-4 text-xs text-slate-300">
              <div className="bg-slate-950 p-4 rounded-lg border border-slate-800 space-y-2">
                <div className="font-extrabold text-white text-sm">1. Executive Summary</div>
                <p className="leading-relaxed">
                  On 2026-09-24T14:30:00Z, an anomalous surface feature classified as Mineral Oil / Bilge Discharge was identified at 19.3347°N, 71.3662°E. Stochastic hydrodynamic back-trajectories estimate a probable origin region and estimated time window between 10:00 UTC and 12:30 UTC. Analysis identified candidate vessels warranting further investigation.
                </p>
              </div>

              <div className="bg-slate-950 p-4 rounded-lg border border-slate-800 space-y-2">
                <div className="font-extrabold text-white text-sm">2. Candidate Vessels Warranting Further Investigation</div>
                <ul className="space-y-2 pl-4 list-disc text-slate-300">
                  <li><strong>NEPTUNE_TRANSIT (IMO-9428812)</strong> — Priority Tier 1: Sailed through probable origin region during estimated window; speed reduction to 7.1 kts.</li>
                  <li><strong>SHADOW_CARRIER (IMO-9102441)</strong> — Priority Tier 1: 42-minute Class-A AIS reception gap across origin boundary.</li>
                </ul>
              </div>

              <div className="bg-slate-950 p-4 rounded-lg border border-slate-800 space-y-2">
                <div className="font-extrabold text-white text-sm">3. Traceable Evidence Vault & Cryptographic Digest</div>
                <p className="font-mono text-[11px] text-slate-400">
                  SHA-256 Digest: <code>a8f5f167f44f4964e6c998dee827110c</code><br />
                  Document Type: Investigation-Ready Evidence Dossier
                </p>
              </div>
            </div>

            <div className="p-4 border-t border-slate-800 flex justify-end gap-3">
              <a
                href={`http://localhost:8000/api/incidents/${caseId}/dossier/html`}
                target="_blank"
                rel="noreferrer"
                className="bg-cyan-600 hover:bg-cyan-500 text-white font-bold text-xs px-4 py-2 rounded transition-colors flex items-center gap-2"
              >
                <span>🖨️</span>
                <span>OPEN PRINTABLE PDF/HTML DOSSIER</span>
              </a>
              <button
                onClick={() => setShowReportModal(false)}
                className="bg-slate-800 hover:bg-slate-700 text-slate-200 font-bold text-xs px-4 py-2 rounded transition-colors"
              >
                Close
              </button>
            </div>
          </div>
        </div>
      )}
    </main>
  );
}
