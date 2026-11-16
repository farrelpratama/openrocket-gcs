
import React from "react";

import { Terminal } from "lucide-react";

export default function EventLog({ entries }) {
  return (
    <section className="panel flex flex-col p-4 lg:col-span-7">

      <h3 className="panel-title flex items-center gap-1.5">
        <Terminal className="h-4 w-4 text-emerald-400" />
        Mission Event Log
      </h3>

      <div className="h-48 flex-1 space-y-1 overflow-y-auto rounded-lg border border-slate-800 bg-slate-950 p-3 font-mono text-xs text-slate-300">

        {entries.map((entry, index) => (
          <div
            key={`${entry}-${index}`}
            className={
              entry.includes("ERROR")
                ? "font-bold text-red-400"
                : entry.includes("finished")
                  ? "text-emerald-400"
                  : "text-slate-300"
            }
          >
            {entry}
          </div>
        ))}

      </div>

    </section>
  );
}
