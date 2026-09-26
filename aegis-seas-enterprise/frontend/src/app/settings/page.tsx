"use client";

import { useState } from 'react';

export default function SettingsPage() {
  const [webhookUrl, setWebhookUrl] = useState('https://mrcc-mumbai.icg.gov.in/api/v1/alerts');
  const [role, setRole] = useState('Investigator');

  return (
    <main className="p-6 max-w-7xl mx-auto w-full space-y-6">
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 flex justify-between items-center">
        <div>
          <h1 className="text-xl font-extrabold text-white flex items-center gap-2">
            <span>⚙️</span> Enterprise System Settings
          </h1>
          <p className="text-slate-400 text-xs mt-1">
            Configure monitored coastal risk zones, alert webhooks, and role-based access control (RBAC).
          </p>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* Role-Based Access Control */}
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 space-y-4">
          <h2 className="font-extrabold text-xs text-slate-200 uppercase tracking-wider border-b border-slate-800 pb-2">
            Role-Based Access Control (RBAC)
          </h2>
          <div className="space-y-3 text-xs">
            <div>
              <label className="block text-slate-400 font-bold mb-1">Active User Role:</label>
              <select
                value={role}
                onChange={(e) => setRole(e.target.value)}
                className="w-full bg-slate-950 border border-slate-800 rounded p-2 text-slate-200 font-semibold"
              >
                <option value="Admin">Administrator (Full Case Management & Configuration)</option>
                <option value="Investigator">Lead Investigator (Case Review & Dossier Generation)</option>
                <option value="Auditor">Compliance Auditor (Read-Only Evidence Vault Access)</option>
              </select>
            </div>
            <div className="bg-slate-950 p-3 rounded text-[11px] text-slate-400 leading-relaxed">
              <strong>Permissions for {role}:</strong> Can review cases, perform spatial-temporal vessel analysis, write investigator case notes, and issue digital evidence dossiers.
            </div>
          </div>
        </div>

        {/* Alert Webhooks */}
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 space-y-4">
          <h2 className="font-extrabold text-xs text-slate-200 uppercase tracking-wider border-b border-slate-800 pb-2">
            Inter-Agency Alert Webhooks
          </h2>
          <div className="space-y-3 text-xs">
            <div>
              <label className="block text-slate-400 font-bold mb-1">MRCC / Statutory Endpoint Webhook:</label>
              <input
                type="text"
                value={webhookUrl}
                onChange={(e) => setWebhookUrl(e.target.value)}
                className="w-full bg-slate-950 border border-slate-800 rounded p-2 font-mono text-slate-200"
              />
            </div>
            <button className="bg-cyan-600 hover:bg-cyan-500 text-white font-bold px-3 py-1.5 rounded transition-colors text-xs">
              Save Webhook Config
            </button>
          </div>
        </div>
      </div>
    </main>
  );
}
