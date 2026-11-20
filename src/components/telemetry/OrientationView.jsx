import React from "react";

import { Compass } from "lucide-react";
import { formatNumber } from "../../utils/formatters";

export default function OrientationView({ latest }) {
  // Ambil data dengan aman menggunakan optional chaining dan default 0
  const roll = latest?.roll ?? 0;
  const pitch = latest?.pitch ?? 0;
  const yaw = latest?.yaw ?? 0;
  const status = latest?.status ?? "NO DATA";

  // Sumbu X MPU sejajar dengan badan roket.
  // Oleh karena itu pitch menggeser bidang horizon secara horizontal.
  const pitchVisual = Math.max(-90, Math.min(90, pitch));

  return (
    <section className="panel flex flex-col items-center justify-between p-4 lg:col-span-5">
      <div className="mb-2 flex w-full items-center justify-between">
        <h3 className="panel-title mb-0 flex items-center gap-1.5">
          <Compass className="h-4 w-4 text-sky-400" />
          Rocket Orientation View
        </h3>

        <span className="text-[11px] font-mono text-slate-400">
          {status}
        </span>
      </div>

      <div className="relative my-2 flex h-44 w-44 items-center justify-center overflow-hidden rounded-full border-4 border-slate-700 bg-sky-900/40 shadow-inner">
        <div
          className="flex h-64 w-64 flex-row transition-transform duration-100"
          style={{
            transform: `rotate(${-roll}deg) translateX(${-pitchVisual * 1.5}px)`,
          }}
        >
          <div className="flex-1 border-r-2 border-white bg-amber-900/50" />
          <div className="flex-1 bg-sky-600/40" />
        </div>

        <div className="pointer-events-none absolute inset-0 flex items-center justify-center">
          <div className="h-24 w-0.5 bg-emerald-400" />
          <div className="absolute h-2 w-2 rounded-full bg-red-500" />
        </div>
      </div>

      <div className="grid w-full grid-cols-3 rounded-lg border border-slate-800 bg-slate-950 p-2 text-center font-mono text-xs">
        <div>Y: {formatNumber(yaw)}°</div>
        <div>P: {formatNumber(pitch)}°</div>
        <div>R: {formatNumber(roll)}°</div>
      </div>

      <p className="mt-2 text-center text-[10px] text-slate-500">
        Attitude visualization only; not a flight-control instrument.
      </p>
    </section>
  );
}