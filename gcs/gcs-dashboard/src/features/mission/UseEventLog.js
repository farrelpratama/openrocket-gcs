
import { useCallback, useState } from "react";

export function useEventLog() {
  const [eventLog, setEventLog] = useState([]);

  const addLog = useCallback((message, type = "info") => {
    const newEvent = {
      id: Date.now(),
      timestamp: new Date().toLocaleTimeString(),
      message,
      type,
    };

    setEventLog((previous) => [newEvent, ...previous]);
  }, []);

  return {
    eventLog,
    addLog,
  };
}
