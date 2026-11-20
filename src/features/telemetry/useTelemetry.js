import {
  useCallback,
  useEffect,
  useRef,
  useState,
} from "react";

import { generateSimulatedTelemetry } from "./telemetrySimulator";
import { GcsAltitudeFilter } from "./altitudeFilter";

import {
  MAX_TELEMETRY_POINTS,
  REPLAY_INTERVAL_MS,
  SIMULATION_INTERVAL_MS,
} from "../../utils/constants";

export function useTelemetry({ addLog }) {
  const [mode, setMode] = useState("SIMULATION");
  const [isRunning, setIsRunning] = useState(false);
  const [telemetryHistory, setTelemetryHistory] = useState([]);
  const [replayData, setReplayData] = useState([]);
  const [replayIndex, setReplayIndex] = useState(0);

  const startTimeRef = useRef(null);
  const replayIndexRef = useRef(0);
  const altitudeFilterRef = useRef(new GcsAltitudeFilter({
    timeConstantSeconds: 0.30,
    medianWindowSize: 5,
  }));

  const addTelemetry = useCallback((point) => {
    setTelemetryHistory((previous) => [
      ...previous.slice(-(MAX_TELEMETRY_POINTS - 1)),
      point,
    ]);
  }, []);

  const filterAltitudePoint = useCallback((point) => {
    if (!point || typeof point !== "object") {
      return point;
    }

    const rawAltitude = Number(point.altitude);
    if (!Number.isFinite(rawAltitude)) {
      return point;
    }

    const timestampUs = Number(point.timestampUs);
    const filteredAltitude = altitudeFilterRef.current.update(
      rawAltitude,
      Number.isFinite(timestampUs) ? timestampUs : null
    );

    return {
      ...point,
      altitudeRaw: rawAltitude,
      altitude: filteredAltitude === null
        ? rawAltitude
        : Number(filteredAltitude.toFixed(3)),
    };
  }, []);

  // Fungsi khusus untuk menerima data dari port serial ESP32 via Tauri
  const pushSerialData = useCallback((rawPayload) => {
    setMode("SERIAL");
    setIsRunning(true);
    let point = rawPayload;
    if (typeof rawPayload === "string") {
      try {
        point = JSON.parse(rawPayload);
      } catch (e) {
        point = { time: new Date().toLocaleTimeString(), altitude: Number(rawPayload) || 0 };
      }
    }
    addTelemetry(filterAltitudePoint(point));
  }, [addTelemetry, filterAltitudePoint]);

  const stop = useCallback(() => {
    setIsRunning(false);
  }, []);

  const start = useCallback(() => {
    if (mode === "REPLAY" && replayData.length === 0) {
      addLog?.("ERROR: No CSV data loaded for replay.");
      return;
    }

    if (mode === "SIMULATION") {
      startTimeRef.current = Date.now();
    }

    setIsRunning(true);
    addLog?.(`Started ${mode.toLowerCase()} source.`);
  }, [mode, replayData.length, addLog]);

  const clear = useCallback(() => {
    setIsRunning(false);
    setTelemetryHistory([]);
    setReplayIndex(0);
    replayIndexRef.current = 0;
    startTimeRef.current = null;
    altitudeFilterRef.current.reset();
  }, []);

  const loadReplay = useCallback(
    (data, filename = "CSV") => {
      setIsRunning(false);
      setMode("REPLAY");
      setReplayData(data);
      setReplayIndex(0);
      replayIndexRef.current = 0;
      setTelemetryHistory([]);
      altitudeFilterRef.current.reset();

      addLog?.(
        `Loaded ${filename}: ${data.length} telemetry records.`
      );
    },
    [addLog]
  );

  useEffect(() => {
    if (!isRunning || mode === "SERIAL") return undefined;

    const intervalMs =
      mode === "REPLAY"
        ? REPLAY_INTERVAL_MS
        : SIMULATION_INTERVAL_MS;

    const interval = window.setInterval(() => {
      if (mode === "REPLAY") {
        const currentIndex = replayIndexRef.current;
        const point = replayData[currentIndex];

        if (!point) {
          setIsRunning(false);
          addLog?.("CSV replay finished.");
          return;
        }

        addTelemetry(filterAltitudePoint(point));

        const nextIndex = currentIndex + 1;
        replayIndexRef.current = nextIndex;
        setReplayIndex(nextIndex);

        if (nextIndex >= replayData.length) {
          setIsRunning(false);
          addLog?.("CSV replay finished.");
        }

        return;
      }

      if (startTimeRef.current === null) {
        startTimeRef.current = Date.now();
      }

      const elapsed = Number(
        ((Date.now() - startTimeRef.current) / 1000).toFixed(2)
      );

      addTelemetry(filterAltitudePoint(generateSimulatedTelemetry(elapsed)));

      if (elapsed >= 22) {
        setIsRunning(false);
        addLog?.("Simulation sequence finished.");
      }
    }, intervalMs);

    return () => window.clearInterval(interval);
  }, [
    isRunning,
    mode,
    replayData,
    addTelemetry,
    addLog,
  ]);

  const selectMode = useCallback(
    (nextMode) => {
      setIsRunning(false);
      setMode(nextMode);
      setTelemetryHistory([]);
      setReplayIndex(0);
      replayIndexRef.current = 0;
      startTimeRef.current = null;
      altitudeFilterRef.current.reset();

      addLog?.(`Switched to ${nextMode.toLowerCase()} mode.`);
    },
    [addLog]
  );

  return {
    mode,
    selectMode,
    isRunning,
    start,
    stop,
    clear,
    telemetryHistory,

    latest:
      telemetryHistory.length > 0
        ? telemetryHistory[telemetryHistory.length - 1]
        : null,

    replayData,
    replayIndex,
    loadReplay,
    addTelemetry,
    pushSerialData, // <-- Diekspor agar bisa dipanggil di App.jsx
  };
}