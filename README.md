# OpenRocket GCS — Telemetry, Ground Control Station & Pop Test

Avionics firmware and ground software for our OpenRocket model rocket. The rocket's ESP32 reads an IMU, streams telemetry to a second ESP32 at the ground control station (GCS), and a GCS application visualizes the data in real time and lets the operator trigger the **pop test** (payload ejection) remotely.

> **Deadline:** 18 Nov 2026 · **Flight, Pop Test & Presentation Day:** 19 Nov 2026
> **Status:** 🚧 In development — see [Project Status](#project-status).
> Full details: [`brief.md`](brief.md)

---

## Table of Contents
1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Features](#features)
4. [Repository Structure](#repository-structure)
5. [Hardware](#hardware)
6. [Getting Started](#getting-started)
7. [Telemetry & Command Protocol](#telemetry--command-protocol)
8. [Pop Test & Safety](#pop-test--safety)
9. [Team & Ownership](#team--ownership)
10. [Git Workflow](#git-workflow)
11. [Project Status](#project-status)

---

## Overview

| Item | Value |
|---|---|
| Rocket MCU | ESP32 DevKit V1 |
| GCS MCU | ESP32 DevKit V1 (USB serial to laptop) |
| Sensors | IMU (model TBD); barometer recommended (TBD) |
| Firmware | PlatformIO (Arduino framework) |
| GCS software | Python backend + web dashboard (assumption, to be confirmed) |
| Target range / altitude | ~1 km |
| Radio link | ⚠️ TBD — ESP-NOW (long-range mode / external antenna) **or** LoRa |

---

## System Architecture

```
┌─────────────────────────────┐                        ┌──────────────────────────────┐
│          ROCKET             │   Telemetry (downlink) │       GROUND CONTROL         │
│                             │                        │                              │
│  IMU (+ baro) ─► ESP32 ─────┼───────────────────────►│ ESP32 GCS ──USB──► Laptop    │
│                   │         │◄───────────────────────┼── Commands (uplink)  (GCS    │
│                   ▼         │   ARM / DISARM / POP   │                      App)    │
│    MOSFET ─► Ejection charge│                        │                        ▲     │
└─────────────────────────────┘                        │                     Operator │
                                                       └──────────────────────────────┘
```

**Data flow:** sensor → Rocket ESP32 → radio → GCS ESP32 → USB serial → backend (parse, validate, log) → WebSocket → dashboard.
**Command flow:** dashboard → backend → USB serial → GCS ESP32 → radio → Rocket ESP32 → ACK back to the dashboard.

---

## Features

- [ ] IMU data acquisition with a stable sample rate
- [ ] Telemetry downlink (sequence number + CRC16)
- [ ] Real-time dashboard: acceleration, gyro, altitude, pressure, battery, link quality, flight state
- [ ] Remote commands: `ARM`, `DISARM`, `POP`, `PING` with ACK/timeout
- [ ] Pop test with multi-layer safety interlocks
- [ ] Data logging (CSV/JSONL) and replay
- [ ] Telemetry simulator (develop the GCS without hardware)
- [ ] Post-flight plotting script

---

## Repository Structure

```
openrocket-gcs/
├── README.md
├── brief.md                  # project brief, roles, timeline
├── docs/                     # architecture, protocol, wiring, test plan
├── shared/protocol/          # packets.h / packets.py — source of truth
├── firmware/
│   ├── rocket/               # PlatformIO — Rocket ESP32
│   │   └── src/{sensors,comms,pyro,state,utils}/
│   └── gcs_bridge/           # PlatformIO — GCS ESP32 (radio ↔ USB)
├── gcs/
│   ├── backend/              # serial reader, command sender, logger, WebSocket
│   └── frontend/             # dashboard (HTML/JS/CSS)
├── hardware/                 # bom.md, schematics, pcb notes
├── data/{logs,samples}/
├── tools/                    # fake_telemetry.py, plot_log.py
└── presentation/             # slides, demo-script.md
```

---

## Hardware

Full parts list and open gaps: [`hardware/bom.md`](hardware/bom.md) and `brief.md` §4.

**Electronics (programmer division):** 2× ESP32 DevKit V1, IMU, perfboard, logic-level MOSFET (ejection switch), resistors, capacitors, Schottky diodes, pin headers, jumper wires.

**Still pending:** battery & regulator, igniter, barometer, arming switch, status LED/buzzer, radio module (if LoRa), avionics bay dimensions.

> Pinouts and wiring tables live in [`docs/wiring.md`](docs/wiring.md) (to be filled in as the circuit is built).

---

## Getting Started

> Commands below are the planned workflow; they become valid as each component lands in the repo.

### Prerequisites
- [VS Code](https://code.visualstudio.com/) + [PlatformIO extension](https://platformio.org/) (or PlatformIO CLI)
- Python 3.10+
- USB cable and driver for the ESP32 (CP210x / CH340, depending on the board)

### 1. Clone
```bash
git clone <repo-url> openrocket-gcs
cd openrocket-gcs
```

### 2. Flash the firmware
```bash
# Rocket ESP32
pio run -d firmware/rocket -t upload

# GCS ESP32
pio run -d firmware/gcs_bridge -t upload

# Serial monitor (example)
pio device monitor -d firmware/gcs_bridge -b 115200
```
Set the peer MAC addresses and pins in `firmware/rocket/include/config.h` (and the GCS equivalent) before flashing.

### 3. Run the GCS
```bash
cd gcs/backend
python -m venv .venv
source .venv/bin/activate        # Windows: .venv\Scripts\activate
pip install -r requirements.txt

python main.py --port COM3       # Linux/macOS: --port /dev/ttyUSB0
```
Open the dashboard at `http://localhost:8000` (default, may change).

### 4. Develop without hardware
```bash
python tools/fake_telemetry.py   # simulated telemetry for backend + dashboard
```

---

## Telemetry & Command Protocol

Defined in `shared/protocol/packets.h` (C) and mirrored in `packets.py`. **Any change must be announced to the whole team.** Details in [`docs/protocol.md`](docs/protocol.md).

| Packet | Direction | Purpose |
|---|---|---|
| Telemetry (`type 0x01`) | Rocket → GCS | seq, uptime, state, IMU, pressure/altitude, temp, battery, flags, CRC16 |
| Command (`ARM` / `DISARM` / `POP` / `PING`) | GCS → Rocket | seq, cmd, CRC16 |
| ACK | Rocket → GCS | cmd, seq, status |

Rocket flight states: `SAFE → ARMED → ASCENT → EJECTED → DESCENT → LANDED`

---

## Pop Test & Safety

The pop test controls an ejection charge. **Treat it as hazardous at all times.**

**Firmware/electronics interlocks:**
- Power-on default is **SAFE**; the MOSFET gate has a **pull-down resistor** so the output stays OFF during boot/reset.
- A **physical arming switch/jumper** in series with the ejection power path.
- Firing requires: `ARMED` state + valid CRC + valid sequence number; output is a fixed short pulse, then auto-return to `SAFE`.
- `ARMED` times out automatically; link loss or invalid state ⇒ stay `SAFE`.
- GCS requires a **two-step confirmation** for `POP`.

**Procedure rules:**
- During development, use an **LED/resistor as a dummy load** — never a live charge on the bench.
- Live tests only under the **team manager's supervision**, following the manager's safety procedure, with everyone clear of the rocket.
- No live charge is connected until the final supervised test.

---

## Team & Ownership

5 programmers: 2 hardware (HP) + 3 software (SP). Names TBD.

| Role | Member | Owns |
|---|---|---|
| HP-1 — Rocket sensing & firmware lead | _TBD_ | `firmware/rocket/` (sensors, state, utils) |
| HP-2 — Comms, pyro & power | _TBD_ | `firmware/gcs_bridge/`, `firmware/rocket/{comms,pyro}`, `hardware/` |
| SP-1 — GCS backend & protocol | _TBD_ | `gcs/backend/`, `shared/protocol/` (with HP-2) |
| SP-2 — GCS frontend / dashboard | _TBD_ | `gcs/frontend/` |
| SP-3 — Logging, simulation & QA | _TBD_ | `data/`, `tools/`, `docs/test-plan.md`, `presentation/` |

---

## Git Workflow

- `main` always builds and works; do not push directly to it.
- One branch per feature: `feat/imu-driver`, `feat/gcs-dashboard`, `fix/crc-mismatch`.
- Open a Pull Request; at least **1 reviewer** from another member before merge.
- Commit messages: short, imperative (`Add IMU driver`, `Fix packet seq overflow`).
- Do not commit logs larger than a few MB or secrets; keep sample logs in `data/samples/`.

---

## Project Status

| Milestone | Target | Status |
|---|---|---|
| Repo + protocol v0.1 + simulator | Thu 12 Nov | ⬜ |
| Sensors read; radio link works; GCS shows fake data | Fri 13 Nov | ⬜ |
| End-to-end telemetry (sensor → dashboard) | Sat 14 Nov | ⬜ |
| Pop test command end-to-end (dummy load) | Sun 15 Nov | ⬜ |
| Soldered avionics board on battery | Mon 16 Nov | ⬜ |
| Integration in rocket + feature freeze | Tue 17 Nov | ⬜ |
| **Deadline** + supervised pop-test trial | Wed 18 Nov | ⬜ |
| **Flight, pop test & presentation** | Thu 19 Nov | ⬜ |

**Open decisions:** radio link for ~1 km (ESP-NOW vs LoRa), barometer, battery & igniter spec, avionics bay dimensions, ground vs in-flight pop test. See `brief.md` §11.2.