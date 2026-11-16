
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

export default function AltitudeChart({ data }) {
  return (
    <section className="panel p-4">

      <h3 className="panel-title">
        Barometric Altitude Profile
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
              dataKey="altitude"
              stroke="#34d399"
              dot={false}
              strokeWidth={2.5}
              name="Altitude (m)"
              isAnimationActive={false}
            />

          </LineChart>

        </ResponsiveContainer>
      </div>

    </section>
  );
}
