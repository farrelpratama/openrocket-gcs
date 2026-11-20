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
    key: "batteryVoltage",
    label: "BATTERY",
    unit: "V",
    color: "text-cyan-400",
  },
  {
    key: "rssi",
    label: "RSSI SIGNAL",
    unit: "dBm",
    color: "text-violet-400",
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
  const currentBattery = latest?.batteryVoltage ?? 0;
  const isLowBattery = currentBattery > 0 && currentBattery < 3.7;

  return (
    <div className="grid grid-cols-2 gap-3 sm:grid-cols-4 xl:grid-cols-8 mb-4">
      {cards.map((card) => {
        // Logika khusus untuk kartu baterai agar menyala merah jika di bawah 3.7V
        const isBatteryCard = card.key === "batteryVoltage";
        const customBorder = isBatteryCard && isLowBattery 
          ? "border-rose-500/60 bg-rose-950/20 animate-pulse" 
          : "border-slate-800 bg-slate-900";

        return (
          <div
            key={card.key}
            className={`rounded-xl border p-3 flex flex-col justify-between shadow-lg ${customBorder}`}
          >
            <p className="text-[11px] text-slate-400 uppercase tracking-wider">
              {card.label}
            </p>

            <p className={`mt-1 font-mono text-lg font-bold ${isBatteryCard && isLowBattery ? "text-rose-400" : card.color}`}>
              {card.key === "time"
                ? formatMissionTime(latest?.[card.key])
                : isBatteryCard
                ? Number(latest?.[card.key] ?? 0).toFixed(2)
                : formatNumber(latest?.[card.key])}

              <span className="text-xs font-normal text-slate-500">
                {" "}{card.unit}
              </span>
            </p>
          </div>
        );
      })}
    </div>
  );
}