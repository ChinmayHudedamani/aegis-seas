"use client";

import Link from 'next/link';
import { usePathname } from 'next/navigation';

export default function Navbar() {
  const pathname = usePathname();

  const navItems = [
    { label: 'Overview Dashboard', href: '/' },
    { label: 'Incident Workspace', href: '/cases/AEGIS-00124' },
    { label: 'Evidence Vault', href: '/vault' },
    { label: 'Investigation Reports', href: '/reports' },
    { label: 'Enterprise Settings', href: '/settings' },
    { label: 'Enterprise Pricing', href: '/enterprise' },
  ];

  return (
    <header className="bg-slate-900 border-b border-slate-800 text-slate-100 px-6 py-3 flex items-center justify-between sticky top-0 z-50">
      <div className="flex items-center gap-4">
        <div className="flex items-center gap-2">
          <div className="w-4 h-4 rounded-full bg-cyan-500 animate-pulse"></div>
          <span className="font-extrabold text-lg tracking-wider text-white">AEGIS-SEAS</span>
          <span className="text-xs font-bold px-2 py-0.5 rounded bg-cyan-950 text-cyan-400 border border-cyan-800">
            ENTERPRISE
          </span>
        </div>
        <div className="hidden lg:block text-xs text-slate-400 border-l border-slate-700 pl-4">
          <span className="font-semibold text-slate-200">Positioning:</span> &ldquo;Don&apos;t just detect the spill. Reconstruct the incident.&rdquo;
        </div>
      </div>

      <nav className="flex items-center gap-1">
        {navItems.map((item) => {
          const isActive = pathname === item.href || (item.href !== '/' && pathname?.startsWith(item.href));
          return (
            <Link
              key={item.href}
              href={item.href}
              className={`px-3 py-1.5 rounded-md text-xs font-semibold transition-colors ${
                isActive
                  ? 'bg-cyan-600 text-white shadow'
                  : 'text-slate-300 hover:bg-slate-800 hover:text-white'
              }`}
            >
              {item.label}
            </Link>
          );
        })}
      </nav>

      <div className="flex items-center gap-3">
        <span className="text-xs text-slate-400 font-mono hidden md:inline">
          Investigator: <strong className="text-slate-200">M. Henderson</strong>
        </span>
        <span className="text-xs px-2.5 py-1 rounded bg-emerald-950 text-emerald-400 font-bold border border-emerald-800">
          AIR-GAPPED READY
        </span>
      </div>
    </header>
  );
}
