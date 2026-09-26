"use client";

import Link from 'next/link';
import { useState } from 'react';

export default function OverviewDashboard() {
  const [incidents] = useState([
    {
      id: 'AEGIS-00124',
      title: 'Offshore Sector 4 Unidentified Hydrocarbon Anomaly',
      coordinates: '19.3347°N, 71.3662°E',
      recorded_at: '2026-09-24T14:30:00Z',
      type: 'Mineral Oil / Bilge Discharge',
      severity: 'High',
      status: 'In Review',
      investigator: 'M. Henderson',
      candidate_count: 3,
    },
    {
      id: 'AEGIS-00125',
      title: 'Gulf of Kutch SPM Terminal Surface Sheen Anomaly',
      coordinates: '22.4500°N, 69.3500°E',
      recorded_at: '2026-09-25T08:15:00Z',
      type: 'Unknown Sheen',
      severity: 'Medium',
      status: 'Active Alert',
      investigator: 'R. Vance',
      candidate_count: 1,
    }
  ]);

  const monitoredZones = [
    { name: 'Mumbai High Offshore Basin', center: '19.35°N, 71.35°E', status: 'Active Surveillance', risk: 'Elevated' },
    { name: 'Gulf of Kutch Refinery Approach', center: '22.45°N, 69.35°E', status: 'Active Surveillance', risk: 'Moderate' },
    { name: 'Southern Shipping Highway', center: '5.95°N, 80.55°E', status: 'Continuous Radar Pass', risk: 'High Density' },
  ];

  const auditLogs = [
    { timestamp: '14:35 UTC', action: 'Case AEGIS-00124 created via automated Sentinel-1 SAR ingestion pass.' },
    { timestamp: '14:42 UTC', action: 'Hydrodynamic RK4 advection model completed 5,000 particle hindcast.' },
    { timestamp: '15:10 UTC', action: 'Investigator M. Henderson added analysis note & updated case status to In Review.' },
  ];

  return (
    <main className="p-6 max-w-7xl mx-auto w-full space-y-6">
      {/* Top Banner */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 flex flex-col md:flex-row justify-between items-start md:items-center gap-4">
        <div>
          <h1 className="text-2xl font-extrabold text-white tracking-tight">
            Commercial Maritime Incident Intelligence Dashboard
          </h1>
          <p className="text-slate-400 text-sm mt-1">
            Don&apos;t just detect the spill. Reconstruct the incident with traceable, investigation-ready evidence.
          </p>
        </div>
        <Link
          href="/cases/AEGIS-00124"
          className="bg-cyan-600 hover:bg-cyan-500 text-white font-bold text-xs px-4 py-2.5 rounded-lg transition-colors shadow-lg shadow-cyan-950/50 flex items-center gap-2"
        >
          <span>OPEN ACTIVE INCIDENT WORKSPACE</span>
          <span>&rarr;</span>
        </Link>
      </div>

      {/* Metrics Row */}
      <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
          <div className="text-xs font-semibold text-slate-400 uppercase tracking-wider">Active Cases</div>
          <div className="text-3xl font-extrabold text-cyan-400 mt-1">2 Cases</div>
          <div className="text-xs text-slate-500 mt-1">1 In Review, 1 Active Alert</div>
        </div>

        <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
          <div className="text-xs font-semibold text-slate-400 uppercase tracking-wider">Candidate Vessels</div>
          <div className="text-3xl font-extrabold text-yellow-400 mt-1">4 Vessels</div>
          <div className="text-xs text-slate-500 mt-1">Warranting further investigation</div>
        </div>

        <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
          <div className="text-xs font-semibold text-slate-400 uppercase tracking-wider">Monitored Risk Zones</div>
          <div className="text-3xl font-extrabold text-emerald-400 mt-1">3 Zones</div>
          <div className="text-xs text-slate-500 mt-1">24/7 Satellite Radar Coverage</div>
        </div>

        <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
          <div className="text-xs font-semibold text-slate-400 uppercase tracking-wider">Audit Compliance</div>
          <div className="text-3xl font-extrabold text-emerald-400 mt-1">100%</div>
          <div className="text-xs text-slate-500 mt-1">SHA-256 Evidence Chain Sealed</div>
        </div>
      </div>

      {/* Main Grid: Active Incidents & Monitored Zones */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left 2 Cols: Incident Table */}
        <div className="lg:col-span-2 bg-slate-900 border border-slate-800 rounded-xl p-5 space-y-4">
          <div className="flex justify-between items-center border-b border-slate-800 pb-3">
            <h2 className="font-extrabold text-slate-100 text-sm uppercase tracking-wider flex items-center gap-2">
              <span className="w-2.5 h-2.5 rounded-full bg-cyan-400"></span>
              Active Incident Investigation Cases
            </h2>
            <span className="text-xs text-slate-400 font-mono">Showing {incidents.length} Records</span>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs border-collapse">
              <thead>
                <tr className="bg-slate-950 text-slate-400 border-b border-slate-800">
                  <th className="p-3 font-semibold">Case ID</th>
                  <th className="p-3 font-semibold">Title / Location</th>
                  <th className="p-3 font-semibold">Type</th>
                  <th className="p-3 font-semibold">Severity</th>
                  <th className="p-3 font-semibold">Status</th>
                  <th className="p-3 font-semibold text-right">Action</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800/60">
                {incidents.map((c) => (
                  <tr key={c.id} className="hover:bg-slate-800/40 transition-colors">
                    <td className="p-3 font-bold font-mono text-cyan-400">{c.id}</td>
                    <td className="p-3">
                      <div className="font-semibold text-slate-200">{c.title}</div>
                      <div className="text-slate-500 text-[11px]">{c.coordinates}</div>
                    </td>
                    <td className="p-3 text-slate-300">{c.type}</td>
                    <td className="p-3">
                      <span className={`px-2 py-0.5 rounded font-bold text-[10px] ${
                        c.severity === 'High' ? 'bg-red-950 text-red-400 border border-red-800' : 'bg-yellow-950 text-yellow-400 border border-yellow-800'
                      }`}>
                        {c.severity}
                      </span>
                    </td>
                    <td className="p-3">
                      <span className="px-2 py-0.5 rounded font-semibold text-[10px] bg-slate-800 text-slate-300">
                        {c.status}
                      </span>
                    </td>
                    <td className="p-3 text-right">
                      <Link
                        href={`/cases/${c.id}`}
                        className="bg-slate-800 hover:bg-cyan-600 text-slate-200 hover:text-white px-2.5 py-1 rounded text-[11px] font-bold transition-colors inline-block"
                      >
                        Investigate &rarr;
                      </Link>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>

        {/* Right Col: Monitored Risk Zones & Recent Audit Logs */}
        <div className="space-y-6">
          {/* Monitored Zones */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 space-y-3">
            <h2 className="font-extrabold text-slate-100 text-xs uppercase tracking-wider flex items-center gap-2 border-b border-slate-800 pb-3">
              <span>🌊</span> Monitored Coastal Risk Zones
            </h2>
            <div className="space-y-2">
              {monitoredZones.map((z, idx) => (
                <div key={idx} className="bg-slate-950 p-3 rounded-lg border border-slate-800/80 flex justify-between items-center text-xs">
                  <div>
                    <div className="font-semibold text-slate-200">{z.name}</div>
                    <div className="text-slate-500 font-mono text-[10px]">{z.center}</div>
                  </div>
                  <span className="px-2 py-0.5 rounded font-semibold text-[10px] bg-emerald-950 text-emerald-400 border border-emerald-800">
                    {z.risk}
                  </span>
                </div>
              ))}
            </div>
          </div>

          {/* Audit Logs */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 space-y-3">
            <h2 className="font-extrabold text-slate-100 text-xs uppercase tracking-wider flex items-center gap-2 border-b border-slate-800 pb-3">
              <span>📜</span> Recent Case Audit Log
            </h2>
            <div className="space-y-2 text-xs">
              {auditLogs.map((log, idx) => (
                <div key={idx} className="border-l-2 border-cyan-500 pl-3 py-1">
                  <span className="font-mono text-slate-400 text-[10px]">{log.timestamp}</span>
                  <p className="text-slate-300 mt-0.5 leading-snug">{log.action}</p>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </main>
  );
}
