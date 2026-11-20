import { useCallback, useState } from "react";

export function useEventLog() {
  const [eventLog, setEventLog] = useState([]);

  const addLog = useCallback((message, type = "info") => {
    const newEvent = {
      id: Date.now() + Math.random(), // Menggunakan random kecil untuk menghindari duplikat ID jika interval sangat cepat
      timestamp: new Date().toLocaleTimeString(),
      message,
      type,
    };

    setEventLog((previous) => [newEvent, ...previous]);
  }, []);

  // Tambahan fungsi untuk membersihkan event log
  const clearLogs = useCallback(() => {
    setEventLog([]);
  }, []);

  return {
    eventLog,
    addLog,
    clearLogs, // <-- Diekspor agar bisa dipanggil di App.jsx
  };
}