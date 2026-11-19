# Project Progress

## Current focus
This project is currently focused on developing the OpenRocket GCS dashboard and its telemetry flow. The main goal is to provide a real-time ground control interface that can:

- display live rocket telemetry,
- switch between simulation and CSV replay modes,
- visualize attitude and altitude trends,
- monitor system status and safety state,
- show mission event logs for operator awareness.

The current work is centered on the frontend dashboard under `gcs/gcs-dashboard`, with the app structure already organized around a dashboard layout, telemetry cards, charts, and mission log panels.

## Project directory structure
The project structure is currently organized as follows, excluding the experimental `ESPNowTesting` folder from the main tree view:

```text
final-assignment/
├── .gitignore
├── BRIEF.md
├── README.md
├── progress.md
├── docs/
├── firmware/
│   ├── gcs_esp/
│   │   ├── include/
│   │   ├── src/
│   │   ├── test/
│   │   ├── .gitignore
│   │   ├── platformio.ini
│   │   └── ...
│   └── rocket_esp/
│       ├── include/
│       ├── lib/
│       ├── src/
│       ├── test/
│       ├── .gitignore
│       ├── platformio.ini
│       └── ...
├── gcs/
│   └── gcs-dashboard/
│       ├── src/
│       ├── src-tauri/
│       ├── index.html
│       ├── package.json
│       ├── vite.config.js
│       ├── tailwind.config.js
│       └── ...
└── (ESPNowTesting excluded intentionally)
```

## What is already in place
The dashboard app is built with React + Vite and uses a modular structure for telemetry and mission events.

### Core UI structure
- `gcs/gcs-dashboard/src/App.jsx` assembles the overall dashboard layout.
- The app includes:
  - header/status area,
  - telemetry source selector,
  - system status panel,
  - telemetry cards,
  - attitude and altitude charts,
  - orientation view,
  - event log display.

### Data flow and state
- Telemetry data is managed through a custom hook (`useTelemetry`).
- Event logs are managed through `useEventLog`.
- The app supports a `SIMULATION` mode and a `CSV REPLAY` mode, which makes it usable for both live testing and recorded telemetry analysis.

### Dashboard features currently represented
- real-time telemetry summary cards,
- chart-based monitoring for attitude and altitude,
- mission event logging,
- dashboard organization for ground-station operation.

## Current status
The project is in the implementation and integration phase. The dashboard shell and core telemetry visualization are already taking shape, but the system is not yet complete.

The remaining work is mainly around connecting the dashboard to the full rocket telemetry pipeline and validating operational behavior:

- connect the frontend to the backend/serial layer,
- confirm telemetry values and formats from the rocket payload,
- verify the safety and control states during operation,
- test CSV replay and simulation reliability,
- refine the dashboard UX for presentation/demo readiness.

## Immediate next steps
1. Finish end-to-end telemetry integration between backend and dashboard.
2. Validate simulation and CSV replay data to make sure charts and indicators behave correctly.
3. Confirm the safety/status logic for the mission flow.
4. Improve the visual polish and final dashboard presentation.
5. Prepare the project for demo and final testing with the full GCS workflow.

## Summary
The project is currently in active development, with the dashboard frontend progressing well and the overall architecture aligned to the OpenRocket GCS concept. The focus now is to complete telemetry integration and operational validation so the system can function as a proper ground control station for the rocket project.
