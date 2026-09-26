import './globals.css';
import Navbar from '@/components/Navbar';
import { Metadata } from 'next';

export const metadata: Metadata = {
  title: 'AEGIS-SEAS Enterprise — Maritime Incident Intelligence & Reconstruction',
  description: 'Audit-ready, high-stakes maritime incident investigation workspace for maritime authorities, P&I clubs, and compliance teams.',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en">
      <head>
        <link
          rel="stylesheet"
          href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"
        />
      </head>
      <body className="bg-slate-950 text-slate-100 min-h-screen flex flex-col font-sans antialiased">
        <Navbar />
        <div className="flex-1 flex flex-col">{children}</div>
      </body>
    </html>
  );
}
