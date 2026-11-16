
import React from "react";

import {
  formatMissionTime,
  formatNumber,
} from "../../utils/formatters";

const cards = [
  {
    key: "time",
    label: "MISSION TIME",
    unit: "s",
    color: "text-white",
  },
  {
    key: "altitude",
    label: "ALTITUDE",
    unit: "m",
    color: "text-emerald-400",
  },
  {
    key: "pressureHpa",
    label: "PRESSURE",
    unit: "hPa",
    color: "text-amber-400",
  },
  {
    key: "yaw",
    label: "YAW",
    unit: "°",
    color: "text-rose-400",
  },
  {
    key: "pitch",
    label: "PITCH",
    unit: "°",
    color: "text-sky-400",
  },
  {
    key: "roll",
    label: "ROLL",
    unit: "°",
    color: "text-orange-400",
  },
];

export default function TelemetryCards({ latest }) {
  return (
    <div className="grid grid-cols-2 gap-3 sm:grid-cols-3 xl:grid-cols-6">

      {cards.map((card) => (
        <div
          key={card.key}
          className="rounded-xl border border-slate-800 bg-slate-900 p-3"
        >
          <p className="text-[11px] text-slate-400">
            {card.label}
          </p>

          <p className={`mt-1 font-mono text-lg font-bold ${card.color}`}>
            {card.key === "time"
              ? formatMissionTime(latest[card.key])
              : formatNumber(latest[card.key])}

            <span className="text-xs font-normal">
              {" "}{card.unit}
            </span>
          </p>
        </div>
      ))}

    </div>
  );
}
