"use client";

import { useState } from 'react';

export default function CommercialLandingPage() {
  const [showDemoModal, setShowDemoModal] = useState<boolean>(false);
  const [demoForm, setDemoForm] = useState({
    orgType: 'P&I Club / Maritime Insurer',
    fleetSize: '50+ Vessels',
    jurisdiction: 'Indian Ocean / Arabian Sea',
    contactEmail: 'claims@maritime-compliance.org'
  });
  const [submitted, setSubmitted] = useState<bool>(false);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    setSubmitted(true);
  };

  return (
    <main className="p-6 max-w-7xl mx-auto w-full space-y-10 my-4">
      {/* Hero Section */}
      <div className="text-center max-w-4xl mx-auto space-y-4">
        <div className="inline-block px-3 py-1 rounded-full bg-cyan-950 text-cyan-400 border border-cyan-800 text-xs font-bold uppercase tracking-wider">
          Commercial Maritime Investigation Platform
        </div>
        <h1 className="text-4xl md:text-5xl font-extrabold text-white tracking-tight leading-tight">
          Turn Maritime Evidence into <span className="text-cyan-400">Investigation-Ready Intelligence</span>
        </h1>
        <p className="text-slate-300 text-base md:text-lg max-w-2xl mx-auto font-medium">
          Don&apos;t just detect the spill. Reconstruct the incident. High-stakes forensic investigation workspace for maritime authorities, P&I Clubs, harbor police, and legal adjusters.
        </p>
        <div className="pt-2">
          <button
            onClick={() => setShowDemoModal(true)}
            className="bg-cyan-600 hover:bg-cyan-500 text-white font-extrabold text-sm px-6 py-3 rounded-xl shadow-xl shadow-cyan-950/50 transition-all hover:scale-105"
          >
            REQUEST ENTERPRISE DEMO &rarr;
          </button>
        </div>
      </div>

      {/* Value Metrics Row */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 text-center space-y-2">
          <div className="text-4xl font-extrabold text-cyan-400">-85%</div>
          <div className="font-bold text-white text-sm">Investigation Time</div>
          <p className="text-slate-400 text-xs">Shrinks incident reconstruction & origin localization from days to under 2 minutes.</p>
        </div>

        <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 text-center space-y-2">
          <div className="text-4xl font-extrabold text-emerald-400">Zero Blindspots</div>
          <div className="font-bold text-white text-sm">Dark Vessel Kinematics</div>
          <p className="text-slate-400 text-xs">Unmasks non-broadcasting ships using 2D CA-CFAR radar hard targets & Radon wake inversion.</p>
        </div>

        <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 text-center space-y-2">
          <div className="text-4xl font-extrabold text-yellow-400">Audit-Ready</div>
          <div className="font-bold text-white text-sm">Traceable Evidence Dossier</div>
          <p className="text-slate-400 text-xs">SHA-256 sealed canonical payloads formatted strictly for UNCLOS Article 217 & MARPOL Annex I.</p>
        </div>
      </div>

      {/* Commercial Tiers */}
      <div className="space-y-6">
        <div className="text-center space-y-1">
          <h2 className="text-2xl font-extrabold text-white">Enterprise Commercial Tiers</h2>
          <p className="text-slate-400 text-xs">Tailored deployments for sovereign Coast Guards, P&I Insurance Clubs, and Port Authorities.</p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          {/* Tier 1 */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 flex flex-col justify-between space-y-6 hover:border-slate-700 transition-colors">
            <div className="space-y-3">
              <span className="text-xs font-bold text-cyan-400 uppercase tracking-wider">Tier 1</span>
              <h3 className="text-xl font-bold text-white">Annual Enterprise SaaS</h3>
              <p className="text-slate-400 text-xs">Continuous 24/7 Satellite SAR & AIS monitoring with multi-user investigation workspace access.</p>
              <ul className="space-y-2 text-xs text-slate-300 pl-4 list-disc">
                <li>Continuous 24/7 EEZ Coastal Radar Passes</li>
                <li>Unlimited Multi-Investigator Seats</li>
                <li>Automated PDF/JSON-LD Dossier Generation</li>
                <li>Priority API & Webhook Support</li>
              </ul>
            </div>
            <button
              onClick={() => setShowDemoModal(true)}
              className="w-full bg-slate-800 hover:bg-slate-700 text-white font-bold text-xs py-2.5 rounded-lg transition-colors"
            >
              Contact Sales
            </button>
          </div>

          {/* Tier 2 */}
          <div className="bg-slate-900 border-2 border-cyan-500 rounded-xl p-6 flex flex-col justify-between space-y-6 relative shadow-xl shadow-cyan-950/30">
            <div className="absolute -top-3 left-1/2 -translate-x-1/2 bg-cyan-600 text-white text-[10px] font-extrabold px-3 py-0.5 rounded-full uppercase tracking-wider">
              Most Popular for Insurers
            </div>
            <div className="space-y-3">
              <span className="text-xs font-bold text-cyan-400 uppercase tracking-wider">Tier 2</span>
              <h3 className="text-xl font-bold text-white">Pay-Per-Investigation Package</h3>
              <p className="text-slate-400 text-xs">On-demand forensic incident reconstruction for P&I Clubs, claims adjusters, and maritime legal teams.</p>
              <ul className="space-y-2 text-xs text-slate-300 pl-4 list-disc">
                <li>Single-Case Hydrodynamic Hindcasting</li>
                <li>Full Traceable Evidence Vault Packaging</li>
                <li>Candidate Vessel Kinematic Analysis</li>
                <li>No Long-Term Commitment Required</li>
              </ul>
            </div>
            <button
              onClick={() => setShowDemoModal(true)}
              className="w-full bg-cyan-600 hover:bg-cyan-500 text-white font-bold text-xs py-2.5 rounded-lg transition-colors shadow"
            >
              Request On-Demand Package
            </button>
          </div>

          {/* Tier 3 */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 flex flex-col justify-between space-y-6 hover:border-slate-700 transition-colors">
            <div className="space-y-3">
              <span className="text-xs font-bold text-cyan-400 uppercase tracking-wider">Tier 3</span>
              <h3 className="text-xl font-bold text-white">Enterprise On-Premise</h3>
              <p className="text-slate-400 text-xs">100% Air-gapped, zero-cloud deployment for defense forces, sovereign naval bases, and port authorities.</p>
              <ul className="space-y-2 text-xs text-slate-300 pl-4 list-disc">
                <li>Local Bare-Metal C++ Engine Core</li>
                <li>Zero External Cloud Dependency</li>
                <li>Self-Contained Local DB & PDF Engine</li>
                <li>Dedicated Sovereign On-Site Setup</li>
              </ul>
            </div>
            <button
              onClick={() => setShowDemoModal(true)}
              className="w-full bg-slate-800 hover:bg-slate-700 text-white font-bold text-xs py-2.5 rounded-lg transition-colors"
            >
              Request Defense Briefing
            </button>
          </div>
        </div>
      </div>

      {/* Demo Modal */}
      {showDemoModal && (
        <div className="fixed inset-0 z-50 bg-black/80 flex items-center justify-center p-4 backdrop-blur-sm">
          <div className="bg-slate-900 border border-slate-800 rounded-xl max-w-md w-full p-6 space-y-4 shadow-2xl">
            <div className="flex justify-between items-center border-b border-slate-800 pb-3">
              <h3 className="font-extrabold text-white text-base">Request Enterprise Demo</h3>
              <button onClick={() => setShowDemoModal(false)} className="text-slate-400 hover:text-white font-bold text-xl">&times;</button>
            </div>

            {submitted ? (
              <div className="bg-emerald-950 border border-emerald-800 p-4 rounded-lg text-center space-y-2 text-xs text-emerald-300">
                <div className="font-bold text-sm">Demo Request Received!</div>
                <p>Our commercial enterprise team will reach out to schedule your personalized incident reconstruction briefing.</p>
                <button
                  onClick={() => { setSubmitted(false); setShowDemoModal(false); }}
                  className="bg-emerald-800 text-white font-bold px-3 py-1.5 rounded mt-2 inline-block"
                >
                  Close
                </button>
              </div>
            ) : (
              <form onSubmit={handleSubmit} className="space-y-3 text-xs">
                <div>
                  <label className="block text-slate-400 font-bold mb-1">Organization Type:</label>
                  <select
                    value={demoForm.orgType}
                    onChange={(e) => setDemoForm({ ...demoForm, orgType: e.target.value })}
                    className="w-full bg-slate-950 border border-slate-800 rounded p-2 text-slate-200"
                  >
                    <option>P&I Club / Maritime Insurer</option>
                    <option>Maritime Authority / Coast Guard</option>
                    <option>Harbor Police / Port Authority</option>
                    <option>Maritime Law Firm / Adjuster</option>
                  </select>
                </div>

                <div>
                  <label className="block text-slate-400 font-bold mb-1">Fleet / EEZ Area Size:</label>
                  <input
                    type="text"
                    value={demoForm.fleetSize}
                    onChange={(e) => setDemoForm({ ...demoForm, fleetSize: e.target.value })}
                    className="w-full bg-slate-950 border border-slate-800 rounded p-2 text-slate-200"
                  />
                </div>

                <div>
                  <label className="block text-slate-400 font-bold mb-1">Jurisdiction / Coastal Zone:</label>
                  <input
                    type="text"
                    value={demoForm.jurisdiction}
                    onChange={(e) => setDemoForm({ ...demoForm, jurisdiction: e.target.value })}
                    className="w-full bg-slate-950 border border-slate-800 rounded p-2 text-slate-200"
                  />
                </div>

                <div>
                  <label className="block text-slate-400 font-bold mb-1">Contact Email:</label>
                  <input
                    type="email"
                    required
                    value={demoForm.contactEmail}
                    onChange={(e) => setDemoForm({ ...demoForm, contactEmail: e.target.value })}
                    className="w-full bg-slate-950 border border-slate-800 rounded p-2 text-slate-200"
                  />
                </div>

                <div className="pt-2 flex justify-end gap-2">
                  <button
                    type="button"
                    onClick={() => setShowDemoModal(false)}
                    className="bg-slate-800 text-slate-300 font-bold px-3 py-1.5 rounded"
                  >
                    Cancel
                  </button>
                  <button
                    type="submit"
                    className="bg-cyan-600 hover:bg-cyan-500 text-white font-bold px-4 py-1.5 rounded shadow"
                  >
                    Submit Request
                  </button>
                </div>
              </form>
            )}
          </div>
        </div>
      )}
    </main>
  );
}
