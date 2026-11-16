
import React, { useRef, useState } from "react";

import {
  Download,
  FileUp,
  Play,
  RotateCcw,
  Square,
} from "lucide-react";

import { parseTelemetryCSV } from "../../features/csv/csvParser";
import { downloadTelemetryCSV } from "../../features/csv/csvExporter";

export default function TelemetrySource({
  telemetry,
  addLog,
}) {
  const fileRef = useRef(null);

  const [error, setError] = useState("");

  async function handleFileChange(event) {
    const file = event.target.files?.[0];

    if (!file) return;

    setError("");

    try {
      const content = await file.text();

      const parsed = parseTelemetryCSV(content);

      telemetry.loadReplay(parsed, file.name);
    } catch (err) {
      setError(err.message || "Unable to read CSV file.");

      addLog(`CSV import failed: ${err.message}`);
    } finally {
      event.target.value = "";
    }
  }

  function handleExport() {
    const success = downloadTelemetryCSV(
      telemetry.telemetryHistory
    );

    if (!success) {
      addLog("CSV export skipped: no telemetry data available.");
      return;
    }

    addLog("Telemetry history exported to CSV.");
  }

  return (
    <section className="panel p-4">

      <h2 className="panel-title">
        Telemetry Source
      </h2>

      <div className="mb-3 grid grid-cols-2 gap-2">

        <button
          onClick={() => telemetry.selectMode("SIMULATION")}
          className={`mode-button ${
            telemetry.mode === "SIMULATION"
              ? "mode-button-active-blue"
              : ""
          }`}
        >
          Simulation
        </button>

        <button
          onClick={() => telemetry.selectMode("REPLAY")}
          className={`mode-button ${
            telemetry.mode === "REPLAY"
              ? "mode-button-active-amber"
              : ""
          }`}
        >
          CSV Replay
        </button>

      </div>

      {telemetry.mode === "REPLAY" && (
        <div className="mb-3 rounded-lg border border-dashed border-slate-700 bg-slate-950 p-3">

          <button
            onClick={() => fileRef.current?.click()}
            className="flex w-full flex-col items-center justify-center text-xs text-slate-400 transition hover:text-white"
          >
            <FileUp className="mb-1 h-5 w-5 text-amber-400" />

            <span>
              Import telemetry CSV
            </span>
          </button>

          <input
            ref={fileRef}
            type="file"
            accept=".csv,text/csv"
            onChange={handleFileChange}
            className="hidden"
          />

          {telemetry.replayData.length > 0 && (
            <p className="mt-2 text-center text-[11px] text-amber-400">
              Record: {telemetry.replayIndex} /{" "}
              {telemetry.replayData.length}
            </p>
          )}

          {error && (
            <p className="mt-2 text-xs text-red-400">
              {error}
            </p>
          )}

        </div>
      )}

      <div className="flex gap-2">

        <button
          onClick={telemetry.start}
          disabled={telemetry.isRunning}
          className="action-button flex-1 bg-emerald-600 hover:bg-emerald-500 disabled:cursor-not-allowed"
        >
          <Play className="h-4 w-4" />
          START
        </button>

        <button
          onClick={telemetry.stop}
          disabled={!telemetry.isRunning}
          className="action-button flex-1 bg-slate-700 hover:bg-slate-600 disabled:cursor-not-allowed"
        >
          <Square className="h-4 w-4" />
          STOP
        </button>

      </div>

      <button
        onClick={telemetry.clear}
        className="action-button mt-2 w-full border border-slate-700 bg-slate-800 text-slate-200 hover:bg-slate-700"
      >
        <RotateCcw className="h-4 w-4" />
        CLEAR HISTORY
      </button>

      <button
        onClick={handleExport}
        className="action-button mt-2 w-full border border-slate-700 bg-slate-800 text-slate-200 hover:bg-slate-700"
      >
        <Download className="h-4 w-4" />
        EXPORT CSV LOG
      </button>

      <p className="mt-3 text-[11px] leading-relaxed text-slate-500">
        Simulation data is synthetic. Live ESP32 serial input
        will be connected through the Tauri backend later.
      </p>

    </section>
  );
}
