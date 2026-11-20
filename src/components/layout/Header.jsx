
import React from "react";

import { Cpu, Rocket, Wifi } from "lucide-react";

export default function Header({
  connectionStatus = "SIMULATION",
  latest,
}) {
  const isSimulation =
    connectionStatus === "SIMULATION";

  return (
    <header className="flex flex-wrap items-center justify-between gap-4 rounded-xl border border-slate-800 bg-slate-900 p-4 shadow-lg">

      <div className="flex items-center gap-3">
        <div className="rounded-lg border border-blue-500/40 bg-blue-600/20 p-2.5 text-blue-400">
          <Rocket className="h-7 w-7" />
        </div>

        <div>
          <h1 className="text-lg font-bold tracking-wide text-white sm:text-xl">
            GADJAH MADA AEROSPACE — GROUND CONTROL STATION
          </h1>

          <p className="text-xs text-slate-400">
            Telemetry monitoring & flight data review
          </p>
        </div>
      </div>

      <div className="flex flex-wrap items-center gap-2 text-xs">

        <div className="flex items-center gap-2 rounded-lg border border-slate-700 bg-slate-800/80 px-3 py-2">
          <Wifi
            className={`h-4 w-4 ${
              isSimulation
                ? "text-amber-400"
                : "text-emerald-400"
            }`}
          />

          <span>
            SOURCE:{" "}
            <strong
              className={
                isSimulation
                  ? "text-amber-400"
                  : "text-emerald-400"
              }
            >
              {connectionStatus}
            </strong>
          </span>
        </div>

        <div className="flex items-center gap-2 rounded-lg border border-slate-700 bg-slate-800/80 px-3 py-2">
          <Cpu className="h-4 w-4 text-cyan-400" />

          <span>
            NODE:{" "}
            <strong className="text-cyan-400">
              ESP32 TELEMETRY SCHEMA
            </strong>
          </span>
        </div>

        <div className="rounded-lg border border-slate-700 bg-slate-800 px-3 py-2 font-semibold text-slate-300">
          {latest ? "DATA AVAILABLE" : "WAITING FOR DATA"}
        </div>

      </div>
    </header>
  );
}
