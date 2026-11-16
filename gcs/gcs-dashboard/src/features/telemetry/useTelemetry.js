
import {
  useCallback,
  useEffect,
  useRef,
  useState,
} from "react";

import { generateSimulatedTelemetry } from "./telemetrySimulator";

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

  const addTelemetry = useCallback((point) => {
    setTelemetryHistory((previous) => [
      ...previous.slice(-(MAX_TELEMETRY_POINTS - 1)),
      point,
    ]);
  }, []);

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
  }, []);

  const loadReplay = useCallback(
    (data, filename = "CSV") => {
      setIsRunning(false);
      setMode("REPLAY");
      setReplayData(data);
      setReplayIndex(0);
      replayIndexRef.current = 0;
      setTelemetryHistory([]);

      addLog?.(
        `Loaded ${filename}: ${data.length} telemetry records.`
      );
    },
    [addLog]
  );

  useEffect(() => {
    if (!isRunning) return undefined;

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

        addTelemetry(point);

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

      addTelemetry(generateSimulatedTelemetry(elapsed));

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
  };
}
