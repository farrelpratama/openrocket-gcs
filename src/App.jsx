import React, { useEffect, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";

import Header from "./components/layout/Header";
import DashboardLayout from "./components/layout/DashboardLayout";
import TelemetrySource from "./components/telemetry/TelemetrySource";
import SerialPanel from "./components/telemetry/SerialPanel";
import TelemetryCards from "./components/telemetry/TelemetryCards";
import AttitudeChart from "./components/telemetry/AttitudeChart";
import AltitudeChart from "./components/telemetry/AltitudeChart";
import OrientationView from "./components/telemetry/OrientationView";
import EventLog from "./components/mission/EventLog";
import SystemStatus from "./components/safety/SystemStatus";

import { useTelemetry } from "./features/telemetry/useTelemetry";
import { createEmptyTelemetry } from "./types/telemetry";
import { useEventLog } from "./features/mission/UseEventLog";
import {
  ShieldAlert,
  Trash2,
  Zap,
  Lock,
  Unlock,
  Loader2,
  AlertOctagon,
  Battery,
  Wifi,
} from "lucide-react";

export default function App() {
  const { eventLog, addLog, clearLogs } = useEventLog();
  const telemetry = useTelemetry({ addLog });
  const latest = telemetry.latest ?? createEmptyTelemetry();

  const [loadingAction, setLoadingAction] = useState(null);
  const [isArmed, setIsArmed] = useState(false);
  const serialUpdateRef = useRef(0);
  const serialSaveRef = useRef(0);

  // Menangkap telemetri real-time & log status dari backend Rust (serial.rs)
  useEffect(() => {
    let disposed = false;
    let unlistenSerialLine;
    let unlistenTelemetry;
    let unlistenSerialLog;

    async function setupListeners() {
        unlistenSerialLine = await listen("serial-line", async (event) => {
          const line = String(event.payload ?? "").trim();
          const fields = line.split(",").map((value) => value.trim());

          if (fields.length >= 16) {
            try {
              const numbers = fields.slice(0, 16).map(Number);
              if (numbers.every((value) => Number.isFinite(value))) {
                const payload = {
                  sequence: numbers[0],
                  timestampUs: numbers[1],
                  time: numbers[1] / 1_000_000,
                  ax: numbers[2], ay: numbers[3], az: numbers[4],
                  gx: numbers[5], gy: numbers[6], gz: numbers[7],
                  mpuTemp: numbers[8], baroTemp: numbers[9],
                  pressurePa: numbers[10], pressureHpa: numbers[11],
                  altitude: numbers[12], roll: numbers[13],
                  pitch: numbers[14], yaw: numbers[15],
                  rssi: null,
                  batteryVoltage: null,
                  status: "LIVE",
                };

                const now = performance.now();
                if (now - serialUpdateRef.current >= 100) {
                  serialUpdateRef.current = now;
                  telemetry.pushSerialData(payload);
                }

                if (now - serialSaveRef.current >= 100) {
                  serialSaveRef.current = now;
                  invoke("save_telemetry_to_csv", {
                    dataJson: JSON.stringify(payload),
                  }).catch((error) => {
                    console.warn("Gagal menyimpan data serial:", error);
                  });
                }
              }
            } catch (error) {
              console.error("Gagal memproses serial-line:", error);
            }
          }
        });

        unlistenTelemetry = await listen("telemetry-data", () => {});

        unlistenSerialLog = await listen("serial-log", (event) => {
        const msg = String(event.payload ?? "");
        const isError =
          msg.includes("FAILED") ||
          msg.includes("ERROR") ||
          msg.includes("Invalid");
        addLog(`[ESP32] ${msg}`, isError ? "error" : "info");
      });
    }

    setupListeners().catch((error) => {
      console.error("Gagal memasang listener serial:", error);
      addLog(`[Serial Error] ${error}`, "error");
    });

    return () => {
      disposed = true;
      if (unlistenSerialLine) unlistenSerialLine();
      if (unlistenTelemetry) unlistenTelemetry();
      if (unlistenSerialLog) unlistenSerialLog();
    };
  }, [telemetry.pushSerialData, addLog]);

  // Fungsi pengiriman perintah misi / safety / abort ke backend Rust
  const handleMissionCommand = async (commandType, label) => {
    if (commandType === "EJECTION_POP" || commandType === "ABORT") {
      const confirmAction = window.confirm(
        `⚠️ PERINGATAN KRITIS: Apakah Anda yakin ingin mengeksekusi perintah [${label}]?`
      );
      if (!confirmAction) return;
    }

    setLoadingAction(commandType);
    try {
      const response = await invoke("send_mission_command", { commandType });
      addLog(`[Command Success] ${response}`, "success");

      if (commandType === "ARM") setIsArmed(true);
      if (commandType === "DISARM" || commandType === "ABORT") setIsArmed(false);
    } catch (error) {
      console.error(`Gagal mengirim perintah ${label}:`, error);
      addLog(`[Command Error] Gagal ${label}: ${error}`, "error");
    } finally {
      setLoadingAction(null);
    }
  };

  const sourceLabel =
    telemetry.mode === "SIMULATION"
      ? "SIMULATION"
      : telemetry.mode === "SERIAL"
      ? "ESP32 USB SERIAL"
      : "CSV REPLAY";

  const batteryDisplay =
    typeof latest.batteryVoltage === "number"
      ? `${latest.batteryVoltage.toFixed(2)}V`
      : "N/A";

  const rssiDisplay =
    latest.rssi !== null && latest.rssi !== undefined
      ? `${latest.rssi} dBm`
      : "N/A";

  return (
    <DashboardLayout
      header={
        <div className="flex items-center justify-between w-full">
          <Header connectionStatus={sourceLabel} latest={telemetry.latest} />

          {/* Widget Indikator Baterai & Sinyal di Header */}
          <div className="hidden md:flex items-center gap-4 px-4 py-1.5 bg-slate-900 border border-slate-800 rounded-lg text-xs font-mono">
            <div
              className="flex items-center gap-1.5 text-emerald-400"
              title="Onboard Battery Voltage"
            >
              <Battery className="w-4 h-4" />
              <span>{batteryDisplay}</span>
            </div>
            <div className="h-3 w-[1px] bg-slate-700" />
            <div
              className="flex items-center gap-1.5 text-cyan-400"
              title="Wireless RSSI Signal"
            >
              <Wifi className="w-4 h-4" />
              <span>{rssiDisplay}</span>
            </div>
          </div>
        </div>
      }
      sidebar={
        <>
          <SerialPanel
            onConnectionChange={(connected, port) => {
              if (connected) telemetry.selectMode("SERIAL");
              addLog(
                connected
                  ? `Terhubung ke port hardware ${port}`
                  : "Koneksi serial diputus.",
                connected ? "success" : "info"
              );
            }}
          />

          <TelemetrySource telemetry={telemetry} addLog={addLog} />
          <SystemStatus
            mode={telemetry.mode}
            isRunning={telemetry.isRunning}
          />

          {/* Panel Kontrol Safety & Emergency Abort */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-lg text-slate-100 flex flex-col gap-3 mt-4">
            <div className="flex items-center justify-between border-b border-slate-800 pb-2">
              <div className="flex items-center gap-2">
                <ShieldAlert className="text-amber-400 w-4 h-4" />
                <h3 className="font-semibold text-xs tracking-wide uppercase text-slate-300">
                  Mission Safety
                </h3>
              </div>
              <span
                className={`text-[10px] px-2 py-0.5 rounded font-bold tracking-wider ${
                  isArmed
                    ? "bg-rose-500/20 text-rose-400 border border-rose-500/40 animate-pulse"
                    : "bg-emerald-500/20 text-emerald-400 border border-emerald-500/30"
                }`}
              >
                {isArmed ? "ARMED" : "SAFE"}
              </span>
            </div>

            <div className="grid grid-cols-2 gap-2">
              {/* Tombol ARM */}
              <button
                onClick={() => handleMissionCommand("ARM", "ARM")}
                disabled={loadingAction !== null || isArmed}
                className="bg-emerald-600/80 hover:bg-emerald-600 active:scale-95 disabled:opacity-40 text-white font-medium py-2 px-3 rounded-lg flex items-center justify-center gap-1.5 transition text-xs shadow cursor-pointer"
              >
                {loadingAction === "ARM" ? (
                  <Loader2 className="w-3.5 h-3.5 animate-spin" />
                ) : (
                  <Lock className="w-3.5 h-3.5" />
                )}
                ARM
              </button>

              {/* Tombol DISARM */}
              <button
                onClick={() => handleMissionCommand("DISARM", "DISARM")}
                disabled={loadingAction !== null || !isArmed}
                className="bg-slate-800 hover:bg-slate-700 active:scale-95 disabled:opacity-40 border border-slate-700 text-slate-200 font-medium py-2 px-3 rounded-lg flex items-center justify-center gap-1.5 transition text-xs cursor-pointer"
              >
                {loadingAction === "DISARM" ? (
                  <Loader2 className="w-3.5 h-3.5 animate-spin" />
                ) : (
                  <Unlock className="w-3.5 h-3.5" />
                )}
                DISARM
              </button>

              {/* Tombol Ejection */}
              <button
                onClick={() =>
                  handleMissionCommand("EJECTION_POP", "EJECTION_POP")
                }
                disabled={loadingAction !== null}
                className="col-span-2 bg-rose-600 hover:bg-rose-500 active:scale-95 disabled:opacity-40 text-white font-semibold py-2 px-3 rounded-lg flex items-center justify-center gap-1.5 transition text-xs shadow-lg shadow-rose-950 cursor-pointer"
              >
                {loadingAction === "EJECTION_POP" ? (
                  <Loader2 className="w-3.5 h-3.5 animate-spin" />
                ) : (
                  <Zap className="w-3.5 h-3.5 text-amber-300" />
                )}
                TRIGGER EJECTION
              </button>

              {/* Tombol Darurat: EMERGENCY ABORT */}
              <button
                onClick={() => handleMissionCommand("ABORT", "EMERGENCY ABORT")}
                disabled={loadingAction !== null}
                className="col-span-2 bg-red-700 hover:bg-red-600 active:scale-95 disabled:opacity-40 text-white font-bold py-2 px-3 rounded-lg flex items-center justify-center gap-1.5 transition text-xs shadow-lg shadow-red-950 border border-red-500/50 cursor-pointer animate-pulse"
              >
                {loadingAction === "ABORT" ? (
                  <Loader2 className="w-3.5 h-3.5 animate-spin" />
                ) : (
                  <AlertOctagon className="w-4 h-4 text-white" />
                )}
                EMERGENCY ABORT
              </button>

              {/* Tombol Clear Log */}
              <button
                onClick={() => {
                  clearLogs?.();
                  addLog("[System] Event log dibersihkan.", "info");
                }}
                className="col-span-2 bg-slate-800/80 hover:bg-slate-700 active:scale-95 border border-slate-700 text-slate-300 font-medium py-1.5 px-3 rounded-lg flex items-center justify-center gap-1.5 transition text-xs cursor-pointer mt-1"
              >
                <Trash2 className="w-3.5 h-3.5 text-rose-400" /> Clear Event Log
              </button>
            </div>
          </div>
        </>
      }
    >
      <TelemetryCards latest={latest} />

      <div className="grid grid-cols-1 gap-4 lg:grid-cols-2">
        <AttitudeChart data={telemetry.telemetryHistory} />
        <AltitudeChart data={telemetry.telemetryHistory} />
      </div>

      <div className="grid grid-cols-1 gap-4 lg:grid-cols-12">
        <OrientationView latest={latest} />

        <div className="lg:col-span-6 flex flex-col">
          <EventLog entries={eventLog} />
        </div>
      </div>
    </DashboardLayout>
  );
}