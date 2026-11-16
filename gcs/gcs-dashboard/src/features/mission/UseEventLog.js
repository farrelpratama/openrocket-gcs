
import { useCallback, useState } from "react";

import {
  MAX_EVENT_LOG_ENTRIES,
} from "../../utils/constants";

export function useEventLog() {
  const [eventLog, setEventLog] = useState(() => [
    `[${new Date().toLocaleTimeString()}] GMA GCS frontend initialized.`,
  ]);

  const addLog = useCallback((message) => {
    const timestamp = new Date().toLocaleTimeString();

    setEventLog((previous) => [
      `[${timestamp}] ${message}`,
      ...previous.slice(0, MAX_EVENT_LOG_ENTRIES - 1),
    ]);
  }, []);

  return {
    eventLog,
    addLog,
  };
}
