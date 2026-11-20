import React, { useEffect, useRef } from "react";
import { Terminal } from "lucide-react";

export default function EventLog({ entries = [] }) {
  const logEndRef = useRef(null);

  // Auto-scroll ke bawah secara otomatis saat ada log baru masuk
  useEffect(() => {
    logEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [entries]);

  const getEntryColor = (entry) => {
    const message = entry.message || entry.text || "";

    if (
      entry.type === "error" ||
      message.includes("CRITICAL") ||
      message.includes("ERROR") ||
      message.includes("ABORT")
    ) {
      return "text-red-400 font-bold bg-red-950/20";
    }

    if (
      entry.type === "warning" ||
      message.includes("ARMED")
    ) {
      return "text-amber-400";
    }

    if (entry.type === "success" || message.includes("Success")) {
      return "text-emerald-400";
    }

    if (message.includes("[Serial]")) {
      return "text-cyan-400";
    }

    return "text-slate-300";
  };

  return (
    <div className="lg:col-span-7 flex flex-col rounded-xl border border-slate-800 bg-slate-900 p-4 shadow-lg h-[280px]">
      <h3 className="mb-2 flex items-center justify-between text-xs font-semibold uppercase tracking-wider text-slate-300">
        <div className="flex items-center gap-1.5">
          <Terminal className="h-4 w-4 text-emerald-400" />
          Mission Event Log ({entries.length})
        </div>
        <span className="text-[10px] font-mono text-slate-500">Live Buffer</span>
      </h3>

      <div className="h-48 flex-1 space-y-1 overflow-y-auto rounded-lg border border-slate-800 bg-slate-950 p-3 font-mono text-xs custom-scrollbar">
        {entries.length === 0 ? (
          <p className="text-slate-500">
            No mission events recorded.
          </p>
        ) : (
          entries.map((entry) => {
            const message = entry.message || entry.text || "";
            const timestamp = entry.timestamp || entry.time || "00:00:00";

            return (
              <div
                key={entry.id || Math.random()}
                className={`py-0.5 px-1 rounded transition-colors ${getEntryColor(entry)}`}
              >
                <span className="text-slate-500 mr-2">[{timestamp}]</span>
                <span>{message}</span>
              </div>
            );
          })
        )}
        <div ref={logEndRef} />
      </div>
    </div>
  );
}