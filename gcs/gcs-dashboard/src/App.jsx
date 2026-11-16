import React, { useState, useEffect, useRef } from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  Legend,
} from "recharts";
import {
  Rocket,
  ShieldAlert,
  ShieldCheck,
  Flame,
  Play,
  Square,
  Upload,
  Download,
  Wifi,
  Cpu,
  Terminal,
  Compass,
} from "lucide-react";

export default function App() {
  const [mode, setMode] = useState("LIVE"); // "LIVE" | "REPLAY"
  const [isRunning, setIsRunning] = useState(false);
  const [armed, setArmed] = useState(false);
  const [confirmArm, setConfirmArm] = useState(false);
  const [confirmPop, setConfirmPop] = useState(false);
  const [ejectionTriggered, setEjectionTriggered] = useState(false);

  const [telemetryHistory, setTelemetryHistory] = useState([]);
  const [replayData, setReplayData] = useState([]);
  const [replayIndex, setReplayIndex] = useState(0);
  const [serialPort, setSerialPort] = useState(null);

  const [eventLog, setEventLog] = useState([
    `[${new Date().toLocaleTimeString()}] GMA GCS Frontend Initialized.`,
  ]);

  const startTimeRef = useRef(null);

  const addLog = (message) => {
    const timeStr = new Date().toLocaleTimeString();
    setEventLog((prev) => [`[${timeStr}] ${message}`, ...prev.slice(0, 49)]);
  };

  // ==========================================
  // HARDWARE WEBSERIAL API CONNECTION
  // ==========================================
  const connectSerial = async () => {
    try {
      if (!navigator.serial) {
        alert("Web Serial API tidak didukung di browser ini. Gunakan Google Chrome atau Microsoft Edge.");
        return;
      }
      const port = await navigator.serial.requestPort();
      await port.open({ baudRate: 115200 });
      setSerialPort(port);
      addLog("SUCCESS: ESP32 GCS Connected via USB Serial.");

      const textDecoder = new TextDecoderStream();
      port.readable.pipeTo(textDecoder.writable);
      const reader = textDecoder.readable.getReader();

      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        if (value) {
          const lines = value.split("\n");
          lines.forEach((line) => {
            const cols = line.trim().split(",");
            if (cols.length >= 6) {
              const newDataPoint = {
                time: parseFloat(cols[0]) || 0,
                yaw: parseFloat(cols[1]) || 0,
                pitch: parseFloat(cols[2]) || 0,
                roll: parseFloat(cols[3]) || 0,
                altitude: parseFloat(cols[4]) || 0,
                voltage: parseFloat(cols[5]) || 11.0,
                rssi: -60,
                status: "LIVE HARDWARE",
              };
              setTelemetryHistory((prev) => [...prev.slice(-99), newDataPoint]);
            }
          });
        }
      }
    } catch (error) {
      addLog(`SERIAL ERROR: ${error.message}`);
    }
  };

  // ==========================================
  // DATA STREAMING / REPLAY LOOP (10 Hz)
  // ==========================================
  useEffect(() => {
    if (!isRunning) return;

    if (!startTimeRef.current) {
      startTimeRef.current = Date.now();
    }

    const interval = setInterval(() => {
      if (mode === "REPLAY") {
        if (replayData.length === 0) {
          addLog("ERROR: No CSV data loaded for replay.");
          setIsRunning(false);
          return;
        }
        setReplayIndex((prevIdx) => {
          if (prevIdx >= replayData.length) {
            setIsRunning(false);
            addLog("CSV Replay finished.");
            return prevIdx;
          }
          const nextPoint = replayData[prevIdx];
          setTelemetryHistory((prevHist) => [...prevHist.slice(-99), nextPoint]);
          return prevIdx + 1;
        });
      } else {
        const elapsed = Number(((Date.now() - startTimeRef.current) / 1000).toFixed(2));
        let status = "STANDBY / PAD";
        let alt = 0;

        if (elapsed >= 2 && elapsed < 7) {
          status = "POWERED FLIGHT (BOOST)";
          alt = Number((45 * Math.pow(elapsed - 2, 1.8)).toFixed(1));
        } else if (elapsed >= 7 && elapsed < 12) {
          status = "COASTING TO APOGEE";
          alt = Number((815 - 4.5 * Math.pow(elapsed - 7, 2)).toFixed(1));
        } else if (elapsed >= 12) {
          status = "EJECTION & DESCENT";
          alt = Math.max(0, Number((702 - 14 * (elapsed - 12)).toFixed(1)));
        }

        if (ejectionTriggered) {
          status = "EJECTION POP TRIGGERED";
        }

        const newDataPoint = {
          time: elapsed,
          yaw: Number((Math.sin(elapsed * 0.6) * 12).toFixed(2)),
          pitch: Number((Math.sin(elapsed * 1.1) * 18).toFixed(2)),
          roll: Number((Math.cos(elapsed * 0.9) * 10).toFixed(2)),
          altitude: alt,
          voltage: armed ? 10.85 : 11.12,
          rssi: -64,
          status: status,
        };

        setTelemetryHistory((prev) => [...prev.slice(-99), newDataPoint]);
      }
    }, 100);

    return () => clearInterval(interval);
  }, [isRunning, mode, replayData, armed, ejectionTriggered]);

  const handleCsvUpload = (e) => {
    const file = e.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (event) => {
      const text = event.target.result;
      const rows = text.split("\n").filter((r) => r.trim() !== "");
      const parsed = rows.slice(1).map((row, idx) => {
        const cols = row.split(",");
        return {
          time: parseFloat(cols[0]) || idx * 0.1,
          yaw: parseFloat(cols[1]) || 0,
          pitch: parseFloat(cols[2]) || 0,
          roll: parseFloat(cols[3]) || 0,
          altitude: parseFloat(cols[4]) || 0,
          voltage: parseFloat(cols[5]) || 11.0,
          rssi: -70,
          status: cols[6] ? cols[6].trim() : "REPLAY_ACTIVE",
        };
      });
      setReplayData(parsed);
      setReplayIndex(0);
      setTelemetryHistory([]);
      addLog(`Loaded CSV Replay: ${file.name} (${parsed.length} frames).`);
    };
    reader.readAsText(file);
  };

  const handleDownloadCsv = () => {
    if (telemetryHistory.length === 0) return;
    const header = "Mission_Time,Yaw,Pitch,Roll,Altitude,Voltage,Status\n";
    const csvRows = telemetryHistory
      .map(
        (d) =>
          `${d.time},${d.yaw},${d.pitch},${d.roll},${d.altitude},${d.voltage},${d.status}`
      )
      .join("\n");
    const blob = new Blob([header + csvRows], { type: "text/csv" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `GMA_GCS_Log_${Date.now()}.csv`;
    a.click();
    addLog("Telemetry data exported to CSV.");
  };

  const executeArm = () => {
    setArmed(true);
    setConfirmArm(false);
    addLog("SYSTEM ARMED. Ejection circuit enabled.");
  };

  const executeDisarm = () => {
    setArmed(false);
    setConfirmArm(false);
    addLog("SYSTEM DISARMED. Safe state.");
  };

  const executePopEjection = () => {
    setEjectionTriggered(true);
    setConfirmPop(false);
    addLog("🚨 CRITICAL: EJECTION POP COMMAND FIRED!");
  };

  const latest = telemetryHistory[telemetryHistory.length - 1] || {
    time: 0,
    yaw: 0,
    pitch: 0,
    roll: 0,
    altitude: 0,
    voltage: 11.1,
    rssi: -65,
    status: "DISARMED / READY",
  };

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 p-4 font-sans">
      <header className="flex flex-wrap items-center justify-between bg-slate-900 border border-slate-800 rounded-xl p-4 mb-4 shadow-lg">
        <div className="flex items-center gap-3">
          <div className="p-2.5 bg-blue-600/20 border border-blue-500/40 rounded-lg text-blue-400">
            <Rocket className="w-7 h-7" />
          </div>
          <div>
            <h1 className="text-xl font-bold tracking-wide text-white">
              GADJAH MADA AEROSPACE — GROUND CONTROL STATION
            </h1>
            <p className="text-xs text-slate-400">
              Module: <span className="text-blue-400 font-mono">gcs/frontend/</span> | Telemetry & Pyrotechnic Recovery Control
            </p>
          </div>
        </div>

        <div className="flex flex-wrap items-center gap-3 mt-2 lg:mt-0">
          <div className="flex items-center gap-2 bg-slate-800/80 px-3 py-1.5 rounded-lg border border-slate-700 text-xs">
            <Wifi className="w-4 h-4 text-emerald-400" />
            <span>LINK: <strong className="text-emerald-400">ONLINE ({latest.rssi} dBm)</strong></span>
          </div>
          <div className="flex items-center gap-2 bg-slate-800/80 px-3 py-1.5 rounded-lg border border-slate-700 text-xs">
            <Cpu className="w-4 h-4 text-cyan-400" />
            <span>NODE: <strong className="text-cyan-400">ESP32 + MPU6050 OK</strong></span>
          </div>
          <div
            className={`px-3 py-1.5 rounded-lg text-xs font-bold border ${
              armed
                ? "bg-red-950/80 border-red-500 text-red-400 animate-pulse"
                : "bg-emerald-950/60 border-emerald-600 text-emerald-400"
            }`}
          >
            {armed ? "● SYSTEM ARMED" : "● SYSTEM DISARMED"}
          </div>
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-4">
        <div className="lg:col-span-3 space-y-4">
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
            <h2 className="text-sm font-semibold text-slate-300 uppercase tracking-wider mb-3">
              Telemetry Source
            </h2>
            
            {/* Tombol Koneksi USB Serial ESP32 */}
            <button
              onClick={connectSerial}
              className="w-full mb-3 py-2 px-3 bg-cyan-600 hover:bg-cyan-500 text-white rounded-lg text-xs font-bold transition flex items-center justify-center gap-2"
            >
              <Cpu className="w-4 h-4" /> Connect ESP32 USB
            </button>

            <div className="grid grid-cols-2 gap-2 mb-3">
              <button
                onClick={() => {
                  setMode("LIVE");
                  setIsRunning(false);
                  addLog("Switched to LIVE Telemetry Mode.");
                }}
                className={`py-2 px-3 rounded-lg text-xs font-semibold border transition ${
                  mode === "LIVE"
                    ? "bg-blue-600 border-blue-500 text-white"
                    : "bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700"
                }`}
              >
                Live Stream
              </button>
              <button
                onClick={() => {
                  setMode("REPLAY");
                  setIsRunning(false);
                  addLog("Switched to CSV Replay Mode.");
                }}
                className={`py-2 px-3 rounded-lg text-xs font-semibold border transition ${
                  mode === "REPLAY"
                    ? "bg-amber-600 border-amber-500 text-white"
                    : "bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700"
                }`}
              >
                CSV Replay
              </button>
            </div>

            {mode === "REPLAY" && (
              <div className="mb-3 p-3 bg-slate-950 border border-dashed border-slate-700 rounded-lg">
                <label className="flex flex-col items-center justify-center cursor-pointer text-xs text-slate-400 hover:text-white">
                  <Upload className="w-5 h-5 mb-1 text-amber-400" />
                  <span>Upload Worst-Case CSV</span>
                  <input type="file" accept=".csv" onChange={handleCsvUpload} className="hidden" />
                </label>
                {replayData.length > 0 && (
                  <p className="text-[11px] text-amber-400 mt-2 text-center">
                    Frame: {replayIndex} / {replayData.length}
                  </p>
                )}
              </div>
            )}

            <div className="flex gap-2">
              <button
                onClick={() => {
                  if (!isRunning && telemetryHistory.length === 0) {
                    startTimeRef.current = Date.now();
                  }
                  setIsRunning(true);
                  addLog(`Started ${mode} telemetry.`);
                }}
                disabled={isRunning}
                className="flex-1 flex items-center justify-center gap-1.5 bg-emerald-600 hover:bg-emerald-500 disabled:opacity-40 text-white py-2 rounded-lg text-xs font-bold transition"
              >
                <Play className="w-4 h-4" /> START
              </button>
              <button
                onClick={() => {
                  setIsRunning(false);
                  addLog("Telemetry stream stopped.");
                }}
                disabled={!isRunning}
                className="flex-1 flex items-center justify-center gap-1.5 bg-slate-700 hover:bg-slate-600 disabled:opacity-40 text-white py-2 rounded-lg text-xs font-bold transition"
              >
                <Square className="w-4 h-4" /> STOP
              </button>
            </div>

            <button
              onClick={handleDownloadCsv}
              className="w-full mt-2 flex items-center justify-center gap-1.5 bg-slate-800 hover:bg-slate-700 border border-slate-700 text-slate-200 py-2 rounded-lg text-xs font-semibold transition"
            >
              <Download className="w-4 h-4" /> Export CSV Log
            </button>
          </div>

          <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
            <h2 className="text-sm font-semibold text-slate-300 uppercase tracking-wider mb-3 flex items-center gap-2">
              <ShieldAlert className="w-4 h-4 text-amber-400" /> Safety & ARM Control
            </h2>

            {!armed ? (
              !confirmArm ? (
                <button
                  onClick={() => setConfirmArm(true)}
                  className="w-full py-2.5 bg-amber-600/20 hover:bg-amber-600/30 border border-amber-500 text-amber-300 rounded-lg text-xs font-bold transition"
                >
                  ARM FLIGHT SYSTEM
                </button>
              ) : (
                <div className="bg-amber-950/50 border border-amber-500/60 p-3 rounded-lg">
                  <p className="text-xs text-amber-200 font-semibold mb-2 text-center">
                    Confirm ARMing pyro & telemetry?
                  </p>
                  <div className="flex gap-2">
                    <button
                      onClick={executeArm}
                      className="flex-1 py-1.5 bg-amber-600 hover:bg-amber-500 text-white rounded text-xs font-bold"
                    >
                      YES, ARM
                    </button>
                    <button
                      onClick={() => setConfirmArm(false)}
                      className="flex-1 py-1.5 bg-slate-700 hover:bg-slate-600 text-white rounded text-xs"
                    >
                      CANCEL
                    </button>
                  </div>
                </div>
              )
            ) : (
              <button
                onClick={executeDisarm}
                className="w-full py-2.5 bg-emerald-600 hover:bg-emerald-500 text-white rounded-lg text-xs font-bold flex items-center justify-center gap-2 transition"
              >
                <ShieldCheck className="w-4 h-4" /> DISARM SYSTEM (SAFE)
              </button>
            )}
          </div>

          <div className="bg-slate-900 border border-red-900/50 rounded-xl p-4">
            <div className="flex items-center justify-between mb-3">
              <h2 className="text-sm font-semibold text-red-400 uppercase tracking-wider flex items-center gap-2">
                <Flame className="w-4 h-4" /> Ejection Charge
              </h2>
              <span className="text-[11px] px-2 py-0.5 rounded bg-slate-800 text-slate-300 font-mono">
                {ejectionTriggered ? "FIRED" : "READY"}
              </span>
            </div>

            {!confirmPop ? (
              <button
                onClick={() => setConfirmPop(true)}
                disabled={!armed && mode === "LIVE"}
                className="w-full py-3 bg-red-600 hover:bg-red-500 disabled:bg-slate-800 disabled:text-slate-500 disabled:border-slate-700 border border-red-400 text-white rounded-lg text-xs font-extrabold tracking-wider transition shadow-lg"
              >
                {armed || mode === "REPLAY"
                  ? "🚨 POP EJECTION CHARGE"
                  : "ARM SYSTEM TO ENABLE POP"}
              </button>
            ) : (
              <div className="bg-red-950/70 border border-red-500 p-3 rounded-lg">
                <p className="text-xs text-red-200 font-bold mb-2 text-center">
                  ⚠️ CONFIRM MANUAL EJECTION POP?
                </p>
                <div className="flex gap-2">
                  <button
                    onClick={executePopEjection}
                    className="flex-1 py-2 bg-red-600 hover:bg-red-500 text-white rounded text-xs font-extrabold"
                  >
                    FIRE NOW!
                  </button>
                  <button
                    onClick={() => setConfirmPop(false)}
                    className="flex-1 py-2 bg-slate-700 hover:bg-slate-600 text-white rounded text-xs"
                  >
                    ABORT
                  </button>
                </div>
              </div>
            )}
          </div>
        </div>

        <div className="lg:col-span-9 space-y-4">
          <div className="grid grid-cols-2 sm:grid-cols-6 gap-3">
            <div className="bg-slate-900 border border-slate-800 p-3 rounded-xl">
              <p className="text-[11px] text-slate-400">MISSION TIME</p>
              <p className="text-lg font-bold font-mono text-white">{latest.time} s</p>
            </div>
            <div className="bg-slate-900 border border-slate-800 p-3 rounded-xl">
              <p className="text-[11px] text-slate-400">ALTITUDE</p>
              <p className="text-lg font-bold font-mono text-emerald-400">{latest.altitude} m</p>
            </div>
            <div className="bg-slate-900 border border-slate-800 p-3 rounded-xl">
              <p className="text-[11px] text-slate-400">BATTERY</p>
              <p className="text-lg font-bold font-mono text-amber-400">{latest.voltage} V</p>
            </div>
            <div className="bg-slate-900 border border-slate-800 p-3 rounded-xl">
              <p className="text-[11px] text-slate-400">YAW</p>
              <p className="text-lg font-bold font-mono text-rose-400">{latest.yaw}°</p>
            </div>
            <div className="bg-slate-900 border border-slate-800 p-3 rounded-xl">
              <p className="text-[11px] text-slate-400">PITCH</p>
              <p className="text-lg font-bold font-mono text-sky-400">{latest.pitch}°</p>
            </div>
            <div className="bg-slate-900 border border-slate-800 p-3 rounded-xl">
              <p className="text-[11px] text-slate-400">ROLL</p>
              <p className="text-lg font-bold font-mono text-orange-400">{latest.roll}°</p>
            </div>
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
            <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
              <h3 className="text-xs font-semibold text-slate-300 uppercase tracking-wider mb-3">
                Attitude Telemetry (Yaw / Pitch / Roll)
              </h3>
              <div className="h-60">
                <ResponsiveContainer width="100%" height="100%">
                  <LineChart data={telemetryHistory}>
                    <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
                    <XAxis dataKey="time" stroke="#64748b" fontSize={11} />
                    <YAxis stroke="#64748b" fontSize={11} />
                    <Tooltip contentStyle={{ backgroundColor: "#0f172a", borderColor: "#334155", fontSize: "12px" }} />
                    <Legend wrapperStyle={{ fontSize: "12px" }} />
                    <Line type="monotone" dataKey="yaw" stroke="#fb7185" dot={false} strokeWidth={2} name="Yaw (°)" />
                    <Line type="monotone" dataKey="pitch" stroke="#38bdf8" dot={false} strokeWidth={2} name="Pitch (°)" />
                    <Line type="monotone" dataKey="roll" stroke="#fb923c" dot={false} strokeWidth={2} name="Roll (°)" />
                  </LineChart>
                </ResponsiveContainer>
              </div>
            </div>

            <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
              <h3 className="text-xs font-semibold text-slate-300 uppercase tracking-wider mb-3">
                Altitude Profile (Meters)
              </h3>
              <div className="h-60">
                <ResponsiveContainer width="100%" height="100%">
                  <LineChart data={telemetryHistory}>
                    <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
                    <XAxis dataKey="time" stroke="#64748b" fontSize={11} />
                    <YAxis stroke="#64748b" fontSize={11} />
                    <Tooltip contentStyle={{ backgroundColor: "#0f172a", borderColor: "#334155", fontSize: "12px" }} />
                    <Legend wrapperStyle={{ fontSize: "12px" }} />
                    <Line type="monotone" dataKey="altitude" stroke="#34d399" dot={false} strokeWidth={2.5} name="Altitude (m)" />
                  </LineChart>
                </ResponsiveContainer>
              </div>
            </div>
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-12 gap-4">
            <div className="lg:col-span-5 bg-slate-900 border border-slate-800 rounded-xl p-4 flex flex-col items-center justify-between">
              <div className="w-full flex items-center justify-between mb-2">
                <h3 className="text-xs font-semibold text-slate-300 uppercase tracking-wider flex items-center gap-1.5">
                  <Compass className="w-4 h-4 text-sky-400" /> Rocket Orientation View
                </h3>
                <span className="text-[11px] font-mono text-slate-400">{latest.status}</span>
              </div>

              <div className="relative w-44 h-44 rounded-full border-4 border-slate-700 overflow-hidden bg-sky-900/40 flex items-center justify-center shadow-inner my-2">
                <div
                  className="w-64 h-64 transition-transform duration-100 flex flex-col"
                  style={{
                    transform: `rotate(${-latest.roll}deg) translateY(${latest.pitch * 1.5}px)`,
                  }}
                >
                  <div className="flex-1 bg-sky-600/40 border-b-2 border-white"></div>
                  <div className="flex-1 bg-amber-900/50"></div>
                </div>
                <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
                  <div className="w-24 h-0.5 bg-emerald-400"></div>
                  <div className="w-2 h-2 rounded-full bg-red-500 absolute"></div>
                </div>
              </div>

              <div className="w-full grid grid-cols-3 text-center text-xs font-mono bg-slate-950 p-2 rounded-lg border border-slate-800">
                <div>Y: {latest.yaw}°</div>
                <div>P: {latest.pitch}°</div>
                <div>R: {latest.roll}°</div>
              </div>
            </div>

            <div className="lg:col-span-7 bg-slate-900 border border-slate-800 rounded-xl p-4 flex flex-col">
              <h3 className="text-xs font-semibold text-slate-300 uppercase tracking-wider mb-2 flex items-center gap-1.5">
                <Terminal className="w-4 h-4 text-emerald-400" /> Mission Event Log
              </h3>
              <div className="flex-1 bg-slate-950 border border-slate-800 rounded-lg p-3 font-mono text-xs text-slate-300 overflow-y-auto h-48 space-y-1">
                {eventLog.map((entry, idx) => (
                  <div
                    key={idx}
                    className={
                      entry.includes("CRITICAL") || entry.includes("ERROR")
                        ? "text-red-400 font-bold"
                        : entry.includes("ARMED")
                        ? "text-amber-400"
                        : "text-slate-300"
                    }
                  >
                    {entry}
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}