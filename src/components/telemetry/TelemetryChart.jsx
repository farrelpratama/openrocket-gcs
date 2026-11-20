import React from 'react';
import { ResponsiveContainer, LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend } from 'recharts';

export default function TelemetryChart({ data }) {
  return (
    <div className="bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-lg text-slate-100 flex flex-col h-80">
      <div className="flex items-center justify-between mb-3">
        <h3 className="font-semibold text-sm text-cyan-400">Flight Telemetry Live Chart</h3>
        <span className="text-xs text-slate-500 font-mono">Real-time Stream</span>
      </div>

      <div className="flex-1 w-full h-full">
        {data.length === 0 ? (
          <div className="h-full flex items-center justify-center text-slate-600 text-xs italic">
            Menunggu data telemetri masuk dari ESP32...
          </div>
        ) : (
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={data} margin={{ top: 5, right: 20, bottom: 5, left: 0 }}>
              <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
              <XAxis dataKey="time" stroke="#64748b" fontSize={11} />
              <YAxis stroke="#64748b" fontSize={11} />
              <Tooltip 
                contentStyle={{ backgroundColor: '#0f172a', borderColor: '#334155', borderRadius: '8px' }}
                labelStyle={{ color: '#94a3b8', fontSize: '11px' }}
                itemStyle={{ color: '#38bdf8', fontSize: '12px' }}
              />
              <Legend wrapperStyle={{ fontSize: '12px', paddingTop: '8px' }} />
              {/* Sesuaikan "altitude" atau "value" dengan properti JSON yang dikirim ESP32 */}
              <Line 
                type="monotone" 
                dataKey="altitude" 
                name="Altitude (m)" 
                stroke="#38bdf8" 
                strokeWidth={2} 
                dot={false} 
                isAnimationActive={false}
              />
            </LineChart>
          </ResponsiveContainer>
        )}
      </div>
    </div>
  );
}