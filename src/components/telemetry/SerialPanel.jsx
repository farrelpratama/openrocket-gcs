import React, { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { RefreshCw, Play, Square, Cpu } from 'lucide-react';

export default function SerialPanel({ onConnectionChange }) {
  const [ports, setPorts] = useState([]);
  const [selectedPort, setSelectedPort] = useState('');
  const [baudRate, setBaudRate] = useState('115200');
  const [isConnected, setIsConnected] = useState(false);
  const [loading, setLoading] = useState(false);

  const handleScanPorts = async () => {
    setLoading(true);
    try {
      const availablePorts = await invoke('list_serial_ports');
      setPorts(availablePorts);
      if (availablePorts.length > 0 && !selectedPort) {
        setSelectedPort(availablePorts[0]);
      }
    } catch (error) {
      console.error("Gagal memindai port serial:", error);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    handleScanPorts();
  }, []);

  const handleConnect = async () => {
    if (!selectedPort) return;
    try {
      await invoke('start_serial_listener', {
        portName: selectedPort,
        baudRate: Number(baudRate),
      });
      setIsConnected(true);
      if (onConnectionChange) onConnectionChange(true, selectedPort);
    } catch (error) {
      console.error("Gagal terhubung:", error);
      alert(`Gagal terhubung: ${error}`);
    }
  };

  const handleDisconnect = async () => {
    try {
      await invoke('stop_serial_listener');
      setIsConnected(false);
      if (onConnectionChange) onConnectionChange(false, null);
    } catch (error) {
      console.error("Gagal memutus koneksi:", error);
    }
  };

  return (
    <div className="bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-lg text-slate-100 flex flex-col gap-4">
      <div className="flex items-center justify-between border-b border-slate-800 pb-3">
        <div className="flex items-center gap-2">
          <Cpu className="text-cyan-400 w-5 h-5" />
          <h2 className="font-semibold text-base">ESP32 Serial Connection</h2>
        </div>
        <span className={`text-xs px-2.5 py-1 rounded-full font-medium ${
          isConnected ? 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30' : 'bg-rose-500/20 text-rose-400 border border-rose-500/30'
        }`}>
          {isConnected ? 'Connected' : 'Disconnected'}
        </span>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
        <div className="flex flex-col gap-1.5">
          <label className="text-xs text-slate-400 font-medium">Serial Port</label>
          <div className="flex gap-2">
            <select
              value={selectedPort}
              onChange={(e) => setSelectedPort(e.target.value)}
              disabled={isConnected}
              className="bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-200 focus:outline-none focus:border-cyan-500 flex-1"
            >
              {ports.length === 0 ? (
                <option value="">Tidak ada port</option>
              ) : (
                ports.map((port) => (
                  <option key={port} value={port}>
                    {port}
                  </option>
                ))
              )}
            </select>
            <button
              onClick={handleScanPorts}
              disabled={isConnected || loading}
              className="bg-slate-800 hover:bg-slate-700 border border-slate-700 p-2 rounded-lg text-slate-300 transition"
              title="Refresh Ports"
            >
              <RefreshCw className={`w-4 h-4 ${loading ? 'animate-spin' : ''}`} />
            </button>
          </div>
        </div>

        <div className="flex flex-col gap-1.5">
          <label className="text-xs text-slate-400 font-medium">Baud Rate</label>
          <select
            value={baudRate}
            onChange={(e) => setBaudRate(e.target.value)}
            disabled={isConnected}
            className="bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-200 focus:outline-none focus:border-cyan-500"
          >
            <option value="9600">9600</option>
            <option value="38400">38400</option>
            <option value="57600">57600</option>
            <option value="115200">115200</option>
            <option value="921600">921600</option>
          </select>
        </div>

        <div className="flex items-end">
          {!isConnected ? (
            <button
              onClick={handleConnect}
              disabled={!selectedPort}
              className="w-full bg-cyan-600 hover:bg-cyan-500 disabled:opacity-50 text-white font-medium py-2 px-4 rounded-lg flex items-center justify-center gap-2 transition shadow-lg shadow-cyan-900/30"
            >
              <Play className="w-4 h-4" /> Connect
            </button>
          ) : (
            <button
              onClick={handleDisconnect}
              className="w-full bg-rose-600 hover:bg-rose-500 text-white font-medium py-2 px-4 rounded-lg flex items-center justify-center gap-2 transition shadow-lg shadow-rose-900/30"
            >
              <Square className="w-4 h-4" /> Disconnect
            </button>
          )}
        </div>
      </div>
    </div>
  );
}