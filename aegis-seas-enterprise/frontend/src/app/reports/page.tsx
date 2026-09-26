"use client";

export default function ReportsPage() {
  const reports = [
    {
      case_id: 'AEGIS-00124',
      title: 'Offshore Sector 4 Unidentified Hydrocarbon Anomaly',
      date: '2026-09-24 15:10 UTC',
      status: 'Confirmed Discharge',
      format: 'PDF / JSON-LD',
      sha256: 'a8f5f167f44f4964e6c998dee827110c',
    },
    {
      case_id: 'AEGIS-00125',
      title: 'Gulf of Kutch SPM Terminal Surface Sheen Anomaly',
      date: '2026-09-25 08:20 UTC',
      status: 'In Review',
      format: 'PDF / JSON-LD',
      sha256: '4b8e912409f8721c128490a12908f01b',
    }
  ];

  return (
    <main className="p-6 max-w-7xl mx-auto w-full space-y-6">
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 flex justify-between items-center">
        <div>
          <h1 className="text-xl font-extrabold text-white flex items-center gap-2">
            <span>📜</span> Generated Investigation Reports
          </h1>
          <p className="text-slate-400 text-xs mt-1">
            Investigation-ready, traceable evidence dossiers formatted for maritime authorities, P&I Clubs, and statutory compliance.
          </p>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {reports.map((r) => (
          <div key={r.case_id} className="bg-slate-900 border border-slate-800 rounded-xl p-5 space-y-3">
            <div className="flex justify-between items-start">
              <div>
                <span className="font-mono text-cyan-400 font-extrabold text-xs">{r.case_id}</span>
                <h3 className="font-bold text-white text-sm mt-0.5">{r.title}</h3>
              </div>
              <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-yellow-950 text-yellow-400 border border-yellow-800">
                {r.status}
              </span>
            </div>

            <div className="text-xs text-slate-400 space-y-1">
              <div>Generated Date: <strong className="text-slate-200">{r.date}</strong></div>
              <div>Digest SHA-256: <code className="text-slate-300">{r.sha256}</code></div>
            </div>

            <div className="pt-2 border-t border-slate-800 flex gap-2">
              <a
                href={`http://localhost:8000/api/incidents/${r.case_id}/dossier/html`}
                target="_blank"
                rel="noreferrer"
                className="bg-cyan-600 hover:bg-cyan-500 text-white font-bold text-xs px-3 py-1.5 rounded transition-colors flex items-center gap-1.5"
              >
                <span>🖨️</span>
                <span>Printable PDF Report</span>
              </a>
              <a
                href={`http://localhost:8000/api/incidents/${r.case_id}/dossier/json`}
                target="_blank"
                rel="noreferrer"
                className="bg-slate-800 hover:bg-slate-700 text-slate-200 font-bold text-xs px-3 py-1.5 rounded transition-colors flex items-center gap-1.5"
              >
                <span>📄</span>
                <span>Export JSON-LD Dossier</span>
              </a>
            </div>
          </div>
        ))}
      </div>
    </main>
  );
}
