
import React from "react";

import {
  ShieldCheck,
  ShieldAlert,
} from "lucide-react";

export default function SystemStatus({
  mode,
  isRunning,
}) {
  const simulation = mode === "SIMULATION";

  return (
    <section className="panel p-4">

      <h2 className="panel-title">
        System Status
      </h2>

      <div className="flex items-center gap-2 text-xs">

        {simulation ? (
          <ShieldAlert className="h-4 w-4 text-amber-400" />
        ) : (
          <ShieldCheck className="h-4 w-4 text-emerald-400" />
        )}

        <span
          className={
            simulation
              ? "text-amber-300"
              : "text-emerald-300"
          }
        >
          {simulation
            ? "SIMULATION MODE"
            : "CSV REPLAY MODE"}
        </span>

      </div>

      <p className="mt-2 text-xs text-slate-400">
        Playback: {isRunning ? "Running" : "Stopped"}
      </p>

      <p className="mt-2 text-[11px] leading-relaxed text-slate-500">
        No arming or pyrotechnic firing commands are
        implemented in this telemetry dashboard.
      </p>

    </section>
  );
}
