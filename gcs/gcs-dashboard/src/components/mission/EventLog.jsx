
import React from "react";
import { Terminal } from "lucide-react";

export default function EventLog({ entries = [] }) {
  const getEntryColor = (entry) => {
    const message = entry.message || "";

    if (
      entry.type === "error" ||
      message.includes("CRITICAL") ||
      message.includes("ERROR")
    ) {
      return "text-red-400 font-bold";
    }

    if (
      entry.type === "warning" ||
      message.includes("ARMED")
    ) {
      return "text-amber-400";
    }

    if (entry.type === "success") {
      return "text-emerald-400";
    }

    return "text-slate-300";
  };

  return (
    <div className="lg:col-span-7 flex flex-col rounded-xl border border-slate-800 bg-slate-900 p-4">
      <h3 className="mb-2 flex items-center gap-1.5 text-xs font-semibold uppercase tracking-wider text-slate-300">
        <Terminal className="h-4 w-4 text-emerald-400" />
        Mission Event Log
      </h3>

      <div className="h-48 flex-1 space-y-1 overflow-y-auto rounded-lg border border-slate-800 bg-slate-950 p-3 font-mono text-xs">
        {entries.length === 0 ? (
          <p className="text-slate-500">
            No mission events recorded.
          </p>
        ) : (
          entries.map((entry) => (
            <div
              key={entry.id}
              className={getEntryColor(entry)}
            >
              [{entry.timestamp}] {entry.message}
            </div>
          ))
        )}
      </div>
    </div>
  );
}
