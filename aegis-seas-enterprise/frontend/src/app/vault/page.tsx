"use client";

import { useState } from 'react';

export default function EvidenceVaultPage() {
  const [filterType, setFilterType] = useState<string>('ALL');

  const evidenceItems = [
    {
      id: 'EV-SAR-001',
      incident_id: 'AEGIS-00124',
      type: 'Satellite SAR',
      timestamp: '2026-09-24T14:30:00Z',
      sensor: 'Sentinel-1 C-Band SAR (IW Swath)',
      sha256: 'a8f5f167f44f4964e6c998dee827110c',
      provider: 'Copernicus Open Access Hub',
      url: '/data/rasters/S1A_IW_20260924.tif'
    },
    {
      id: 'EV-AIS-004',
      incident_id: 'AEGIS-00124',
      type: 'Satellite AIS',
      timestamp: '2026-09-24T14:32:10Z',
      sensor: 'Spire Satellite AIS Constellation',
      sha256: '7c9e12b489a2410ef4129b00881928cf',
      provider: 'Spire Maritime Stream',
      url: '/data/ais/ais_track_AEGIS-00124.csv'
    },
    {
      id: 'EV-MET-002',
      incident_id: 'AEGIS-00124',
      type: 'INCOIS ROMS',
      timestamp: '2026-09-24T12:00:00Z',
      sensor: 'INCOIS ROMS Ocean Current Grid',
      sha256: '12f8491a92e104b2c129088fa12847a9',
      provider: 'Indian National Centre for Ocean Information Services',
      url: '/data/metocean/roms_currents.nc'
    },
    {
      id: 'EV-SAR-002',
      incident_id: 'AEGIS-00125',
      type: 'Satellite SAR',
      timestamp: '2026-09-25T08:15:00Z',
      sensor: 'Sentinel-1 C-Band SAR',
      sha256: '4b8e912409f8721c128490a12908f01b',
      provider: 'Copernicus Open Access Hub',
      url: '/data/rasters/S1A_KUTCH_20260925.tif'
    }
  ];

  const filteredItems = filterType === 'ALL'
    ? evidenceItems
    : evidenceItems.filter(item => item.type === filterType);

  return (
    <main className="p-6 max-w-7xl mx-auto w-full space-y-6">
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 flex justify-between items-center">
        <div>
          <h1 className="text-xl font-extrabold text-white flex items-center gap-2">
            <span>🔐</span> Consolidated Evidence Vault
          </h1>
          <p className="text-slate-400 text-xs mt-1">
            Traceable data sources with cryptographic SHA-256 ingestion checksums for compliance & legal audits.
          </p>
        </div>
        <div className="flex gap-2">
          {['ALL', 'Satellite SAR', 'Satellite AIS', 'INCOIS ROMS'].map((t) => (
            <button
              key={t}
              onClick={() => setFilterType(t)}
              className={`px-3 py-1.5 rounded text-xs font-bold transition-colors ${
                filterType === t
                  ? 'bg-cyan-600 text-white'
                  : 'bg-slate-800 text-slate-300 hover:bg-slate-700'
              }`}
            >
              {t}
            </button>
          ))}
        </div>
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-xl p-5">
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs border-collapse">
            <thead>
              <tr className="bg-slate-950 text-slate-400 border-b border-slate-800">
                <th className="p-3 font-semibold">Evidence ID</th>
                <th className="p-3 font-semibold">Case Reference</th>
                <th className="p-3 font-semibold">Source Type</th>
                <th className="p-3 font-semibold">Timestamp (UTC)</th>
                <th className="p-3 font-semibold">Sensor / Platform</th>
                <th className="p-3 font-semibold">SHA-256 Ingestion Checksum</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60">
              {filteredItems.map((ev) => (
                <tr key={ev.id} className="hover:bg-slate-800/40 transition-colors">
                  <td className="p-3 font-bold font-mono text-cyan-400">{ev.id}</td>
                  <td className="p-3 font-mono text-slate-300">{ev.incident_id}</td>
                  <td className="p-3">
                    <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-slate-800 text-slate-300 border border-slate-700">
                      {ev.type}
                    </span>
                  </td>
                  <td className="p-3 font-mono text-slate-300">{ev.timestamp}</td>
                  <td className="p-3 text-slate-300">{ev.sensor}</td>
                  <td className="p-3 font-mono text-[11px] text-slate-400">
                    <code>{ev.sha256}</code>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </main>
  );
}
