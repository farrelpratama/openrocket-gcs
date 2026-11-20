
import React from "react";

export default function DashboardLayout({
  header,
  sidebar,
  children,
}) {
  return (
    <main className="min-h-screen bg-slate-950 p-4 font-sans text-slate-100">
      {header}

      <div className="mt-4 grid grid-cols-1 gap-4 lg:grid-cols-12">
        <aside className="space-y-4 lg:col-span-3">
          {sidebar}
        </aside>

        <section className="space-y-4 lg:col-span-9">
          {children}
        </section>
      </div>
    </main>
  );
}
