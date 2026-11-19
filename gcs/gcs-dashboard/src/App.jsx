
import React from "react";

import Header from "./components/layout/Header";
import DashboardLayout from "./components/layout/DashboardLayout";

import TelemetrySource from "./components/telemetry/TelemetrySource";
import TelemetryCards from "./components/telemetry/TelemetryCards";
import AttitudeChart from "./components/telemetry/AttitudeChart";
import AltitudeChart from "./components/telemetry/AltitudeChart";
import OrientationView from "./components/telemetry/OrientationView";

import EventLog from "./components/mission/EventLog";
import SystemStatus from "./components/safety/SystemStatus";

import { useEventLog } from "./features/mission/useEventLog";
import { useTelemetry } from "./features/telemetry/useTelemetry";

import { createEmptyTelemetry } from "./types/telemetry";

export default function App() {
  const { eventLog, addLog } = useEventLog();

  const telemetry = useTelemetry({ addLog });

  const latest =
    telemetry.latest ?? createEmptyTelemetry();

  const sourceLabel =
    telemetry.mode === "SIMULATION"
      ? "SIMULATION"
      : "CSV REPLAY";

  return (
    <DashboardLayout
      header={
        <Header
          connectionStatus={sourceLabel}
          latest={telemetry.latest}
        />
      }
      sidebar={
        <>
          <TelemetrySource
            telemetry={telemetry}
            addLog={addLog}
          />

          <SystemStatus
            mode={telemetry.mode}
            isRunning={telemetry.isRunning}
          />
        </>
      }
    >
      <TelemetryCards latest={latest} />

      <div className="grid grid-cols-1 gap-4 lg:grid-cols-2">
        <AttitudeChart
          data={telemetry.telemetryHistory}
        />

        <AltitudeChart
          data={telemetry.telemetryHistory}
        />
      </div>

      <div className="grid grid-cols-1 gap-4 lg:grid-cols-12">
        <OrientationView latest={latest} />

        <EventLog entries={eventLog} />
      </div>
    </DashboardLayout>
  );
}