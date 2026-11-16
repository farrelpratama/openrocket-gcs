
import React from "react";

import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

export default function AttitudeChart({ data }) {
  return (
    <section className="panel p-4">

      <h3 className="panel-title">
        Attitude Telemetry (Yaw / Pitch / Roll)
      </h3>

      <div className="h-60">
        <ResponsiveContainer width="100%" height="100%">

          <LineChart data={data}>

            <CartesianGrid
              strokeDasharray="3 3"
              stroke="#1e293b"
            />

            <XAxis
              dataKey="time"
              stroke="#64748b"
              fontSize={11}
            />

            <YAxis
              stroke="#64748b"
              fontSize={11}
            />

            <Tooltip
              contentStyle={{
                backgroundColor: "#0f172a",
                borderColor: "#334155",
                fontSize: 12,
              }}
            />

            <Legend wrapperStyle={{ fontSize: 12 }} />

            <Line
              type="monotone"
              dataKey="yaw"
              stroke="#fb7185"
              dot={false}
              strokeWidth={2}
              name="Yaw (°)"
              isAnimationActive={false}
            />

            <Line
              type="monotone"
              dataKey="pitch"
              stroke="#38bdf8"
              dot={false}
              strokeWidth={2}
              name="Pitch (°)"
              isAnimationActive={false}
            />

            <Line
              type="monotone"
              dataKey="roll"
              stroke="#fb923c"
              dot={false}
              strokeWidth={2}
              name="Roll (°)"
              isAnimationActive={false}
            />

          </LineChart>

        </ResponsiveContainer>
      </div>

    </section>
  );
}
