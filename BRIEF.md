# Project Brief — OpenRocket Avionics Data, Ground Control Station & Ejection Charge (Pop Test)

> **Division:** Programmer (Hardware + Software) · **Team size:** 5 members
> **Deadline:** 18 November 2026 (Wednesday)
> **Flight, Pop Test & Presentation Day:** 19 November 2026 (Thursday)
> **Status:** DRAFT v0.4 — aligned with the organizer's **Programmer KPI**; hardware fixed at **2 ESP32**. Items marked ⚠️ still need confirmation.

---

## 1. Project Description

### 1.1 Background
Our aerospace team is building an OpenRocket-designed model rocket. The programmer division is responsible for the **avionics electronics, firmware, data path, ground software, and the ejection charge (pop test) control**.

### 1.2 Objectives
1. **Avionics data** — an ESP reads the IMU and produces **yaw, pitch, roll**, and sends it to the **ESP-Laptop** (the ESP connected to the GCS laptop).
2. **CSV storage** — all data can be stored as **CSV** (minimum requirement); CSV can be **forwarded/replayed into the dashboard**.
3. **Ejection charge trigger** — the operator can trigger the ejection charge from the GCS dashboard (pop test), through an ESP that fires it.
4. **Dashboard** — a GCS dashboard visualizes the data and controls the ejection charge.
5. **Bonus targets** — real-time data, a neat and complete dashboard, optional LoRa.
6. **Documentation deliverables** — one GitHub repository per group, and a flowchart (PDF) with explanation.

### 1.3 Scope
| In scope | Out of scope |
|---|---|
| Avionics firmware (IMU → yaw/pitch/roll → CSV/transmit) | Rocket airframe / structure (other divisions) |
| Ejection charge firmware & circuit (MOSFET) | Propulsion / motor selection |
| ESP-Laptop (GCS ESP) firmware | Recovery mechanical design |
| GCS backend + dashboard (live & CSV replay) | |
| GitHub repo, flowchart PDF + explanation, test & docs | |

### 1.4 System Architecture

Two physical ESP32 boards (✅ confirmed by the team):

| Board | Where | Tasks (from KPI) |
|---|---|---|
| **Rocket ESP** | Rocket | **ESP 1 task:** fire the ejection charge on a valid, armed command<br>**ESP 2 task:** read the IMU, send **yaw/pitch/roll**, log CSV |
| **ESP-Laptop (GCS ESP)** | Ground, USB to laptop | Receives avionics data, relays commands, feeds the laptop |

> The KPI labels "ESP 1" and "ESP 2" are treated as **two firmware tasks (modules) running on the single Rocket ESP**. ⚠️ See §13.2 #1.

```
┌──────────────────── ROCKET ─────────────────────┐        ┌────────────── GROUND ───────────────┐
│ Rocket ESP                                      │  YPR   │                                     │
│  ESP 2 task: IMU ─► Yaw/Pitch/Roll ─► CSV (SD)  ├───────►│ ESP-Laptop (GCS ESP)                │
│                                                 │        │     │ USB serial                    │
│  ESP 1 task: MOSFET ─► Ejection charge         │  CMD   │     ▼                               │
│                              ▲                  │◄───────┤ Laptop: backend ─► Dashboard        │
│                              └── ARM / POP ─────┤  + ACK │   (CSV writer)     (live + CSV replay)│
└─────────────────────────────────────────────────┘        │                        ▲ Operator   │
                                                           └─────────────────────────────────────┘
```

**Data flow:** IMU → Rocket ESP → (radio) → ESP-Laptop → USB serial → backend (parse, validate, write CSV) → WebSocket → dashboard.
**Command flow:** dashboard → backend → ESP-Laptop → (radio) → Rocket ESP → ACK back to dashboard.
**Worst-case flow:** no real-time link → the Rocket ESP keeps writing CSV on-board (and/or backend writes CSV) → CSV is imported into the dashboard.

### 1.5 Technical Assumptions ⚠️ (confirm with team)
| Item | Proposed choice | Reason |
|---|---|---|
| Firmware framework | **PlatformIO** ✅ confirmed (Arduino framework) | Reproducible builds, good for a team repo |
| ESP ↔ ESP link | **ESP-NOW** (2.4 GHz, no router) | Native on ESP32, low latency, supports multiple peers |
| ESP-Laptop ↔ laptop | USB Serial (115200–921600 baud) | Simplest, matches KPI wording |
| GCS backend | **Python** (`pyserial` + `FastAPI`/WebSocket) | Fast to build |
| Dashboard | **Web dashboard** (HTML/JS + Chart.js/uPlot) | Live charts, easy to make neat, easy to demo |
| Orientation | IMU fused to yaw/pitch/roll (see §4.1 #2) | KPI requires yaw, pitch, roll |
| Telemetry rate | 10 Hz target (≥ 5 Hz acceptable) | Enough for orientation display |
| LoRa | **Optional bonus**, not required | KPI: LoRa is only extra credit |

> **Range note:** ESP-NOW with the DevKit V1 PCB antenna is typically ~100–250 m in open air. The expected flight altitude is ~1 km, so real-time data may drop during flight. That is acceptable per KPI (real-time is a bonus) **because the CSV fallback exists** — which is why on-board CSV logging is treated as a must-have. LoRa (or ESP-NOW long-range mode + external antenna) only improves the real-time bonus.

---

## 2. Programmer KPI → Priority Tiers

| Tier | KPI item | Acceptance check | Owner |
|---|---|---|---|
| **Tier 1 — Mandatory** | Development repository on GitHub (**one per group**) | Repo exists, all 5 members have commits, README + structure in place | SP-3 (setup), all (commits) |
| | Flowchart (**PDF**) + explanation | PDF in repo, covers system + each node, explanation per flowchart | SP-3 (lead), HP-1/HP-2 (review) |
| | Avionics sensor data enters the ESP-Laptop (CSV storage allowed) | yaw/pitch/roll rows saved to a valid CSV | HP-1, SP-1 |
| | Trigger the ejection charge + visualize in a dashboard | Dashboard button → Rocket ESP fires (dummy load in tests) → ACK shown | HP-2, SP-1, SP-2 |
| **Tier 2 — Plus points** | Data arrives at the ESP-Laptop in **real time** | Live values update on the dashboard at ≥ 5 Hz | HP-1, HP-2, SP-1 |
| | Dashboard is neat | Consistent layout, readable fonts/colors, no clutter | SP-2 |
| | Dashboard display is complete | YPR charts/3D orientation, link status, ESP status, ejection state, CSV import/replay, logs | SP-2, SP-1 |
| **Tier 3 — Optional** | LoRa telemetry (not required; plus for telemetry divisions) | Only if Tier 1 + 2 are done and hardware is available | HP-2 |

**Execution rule:** Tier 1 must be finished by **Sun 15 Nov**. Tier 2 follows. Tier 3 only if everything else is stable.

**Pop test rule:** at the pop test, the **programmer who controls the ejection charge** operates it. Designated **Pop Operator = HP-2** (⚠️ confirm), **backup = HP-1**, both trained on the procedure.

**Worst-case scenario (accepted by KPI):** real-time data cannot be read → data is saved in CSV format, and **the CSV is forwarded to the dashboard** (import/replay). The dashboard must therefore support a CSV import/replay mode from Day 3 on.

---

## 3. Repository Structure (GitHub — one per group)

Legend: **[HP-1] [HP-2]** = Hardware Programmers · **[SP-1] [SP-2] [SP-3]** = Software Programmers · **[ALL]** = shared.

```
openrocket-gcs/
├── README.md                                  [SP-3]
├── brief.md                                   [SP-3]
├── .gitignore                                 [SP-3]  # .pio/, .venv/, __pycache__/, data/logs/*
├── .github/                                   [SP-3]
│   ├── CODEOWNERS                             # folder → owner (auto review request)
│   ├── PULL_REQUEST_TEMPLATE.md
│   └── ISSUE_TEMPLATE/{task.md, bug.md}
│
├── shared/                                    # HW ↔ SW contract — [HP-1][HP-2] + [SP-1]
│   └── protocol/
│       ├── packets.h                          # C structs (source of truth)
│       ├── packets.py                         # Python mirror
│       ├── commands.h / commands.py           # command IDs, state codes, ACK status
│       └── csv_schema.md                      # CSV columns, units, example rows
│
│ ╔══════════════════════ HARDPROG (HP-1, HP-2) ══════════════════════╗
├── firmware/                                  # one PlatformIO project per ESP32 (2 total)
│   ├── rocket_esp/                            # ESP32 #1 — avionics (ESP 2 task) + ejection (ESP 1 task)
│   │   ├── platformio.ini                     # build_flags: -I ../../shared/protocol
│   │   ├── include/{config.h, pins.h}         # pins, rates, peer MAC, ejection pulse time
│   │   ├── src/
│   │   │   ├── main.cpp                       # non-blocking scheduler / FreeRTOS tasks
│   │   │   ├── imu/                           [HP-1]  # IMU driver + calibration
│   │   │   ├── orientation/                   [HP-1]  # yaw / pitch / roll
│   │   │   ├── csv_logger/                    [HP-1]  # SD card CSV (same schema as backend)
│   │   │   ├── telemetry/                     [HP-1]  # build packet (seq, CRC), TX scheduling
│   │   │   ├── comms/                         [HP-2]  # ESP-NOW init, send/receive, link-loss handling
│   │   │   ├── pyro/                          [HP-2]  # ejection: arm/fire logic, MOSFET pulse
│   │   │   ├── state/                         [HP-2]  # SAFE / ARMED / FIRED state machine
│   │   │   └── utils/                         [ALL]   # crc16, timing, ring buffer
│   │   └── test/                              # native unit tests: crc16, state machine, orientation
│   │
│   ├── gcs_esp/                               [HP-2]  # ESP32 #2 — ESP-Laptop: radio ↔ USB serial
│   │   ├── platformio.ini
│   │   ├── include/{config.h, pins.h}
│   │   ├── src/{main.cpp, comms/, serial_link/}   # serial_link = framing packets over USB
│   │   └── test/
│   │
│   └── bench/                                 [HP-1][HP-2]  # tiny standalone sketches for bench testing
│       ├── imu_bench/                         # print raw IMU + yaw/pitch/roll
│       ├── sd_bench/                          # write/read CSV on the SD card
│       ├── mosfet_dummy_load_bench/           # pulse an LED/resistor load (NO live charge)
│       └── espnow_range_bench/                # ping/ACK, RSSI, packet-loss at distance
│
├── hardware/
│   ├── bom.md                                 [HP-2]  # parts list + open gaps
│   ├── wiring/                                [HP-1][HP-2]
│   │   ├── rocket_esp_pinout.md
│   │   ├── gcs_esp_pinout.md
│   │   ├── ejection_circuit.md                # MOSFET, gate pull-down, diode, arming switch
│   │   └── power_budget.md                    # battery, regulator, current at ejection pulse
│   ├── schematics/                            # circuit diagrams (draw.io / KiCad / PNG)
│   ├── pcb/                                   # perfboard layout, assembly photos
│   ├── datasheets/                            # IMU, MOSFET, ESP32 pinout (small PDFs only)
│   ├── checklists/                            [HP-2]
│   │   ├── preflight_checklist.md
│   │   ├── pop_test_procedure.md              # reviewed and approved by the manager
│   │   └── safety_interlocks.md
│   └── test-reports/                          # bench test results (dated, with photos)
│ ╚═══════════════════════════════════════════════════════════════════╝
│
│ ╔══════════════════════ SOFTPROG (SP-1, SP-2, SP-3) ════════════════╗
├── gcs/
│   ├── backend/                               [SP-1]
│   │   ├── requirements.txt
│   │   ├── main.py                            # entry point
│   │   ├── app/
│   │   │   ├── config.py                      # port, baud, paths
│   │   │   ├── serial_link/{reader.py, framing.py}   # read bytes → frames
│   │   │   ├── protocol/{parser.py, models.py}       # CRC check, decode, data models
│   │   │   ├── commands/{sender.py, ack_tracker.py}  # ARM/DISARM/POP/PING + ACK/timeout
│   │   │   ├── storage/{csv_writer.py, csv_import.py, events_logger.py}
│   │   │   ├── session/stats.py               # link status, packet loss, rate
│   │   │   └── api/{ws_server.py, rest.py}    # WebSocket stream; REST: upload CSV, list logs
│   │   └── tests/{test_parser.py, test_csv_roundtrip.py, test_commands.py}
│   │
│   └── frontend/                              [SP-2]
│       ├── index.html
│       ├── css/{theme.css, layout.css}
│       ├── js/
│       │   ├── main.js
│       │   ├── ws_client.js                   # live data connection
│       │   ├── charts.js                      # yaw / pitch / roll over time
│       │   ├── orientation.js                 # attitude indicator / 3D model
│       │   ├── status.js                      # link, battery, flags, rocket state
│       │   ├── controls.js                    # ARM / DISARM / POP + 2-step confirm
│       │   ├── replay.js                      # CSV import / replay (worst-case path)
│       │   └── event_log.js
│       └── assets/{icons/, fonts/}
│
├── tools/                                     [SP-3]
│   ├── fake_telemetry.py                      # simulator (serial / WebSocket)
│   ├── csv_validate.py                        # check a CSV against csv_schema.md
│   └── plot_log.py                            # post-flight plots
│
├── data/                                      [SP-3]
│   ├── samples/{sample_flight.csv, sample_events.csv}   # committed, for dev + demo
│   └── logs/                                  # real runs (git-ignored, small ones copied to samples/)
│
├── docs/                                      [SP-3]  # owners review their parts
│   ├── architecture.md
│   ├── protocol.md                            [SP-1][HP-1][HP-2]
│   ├── test-plan.md
│   ├── flowchart/
│   │   ├── flowchart.pdf                      # KPI deliverable
│   │   ├── flowchart-explanation.md
│   │   └── src/                               # draw.io / Mermaid sources
│   └── images/
│
└── presentation/{slides/, demo-script.md}     [SP-3]
  ╚═══════════════════════════════════════════════════════════════════╝
```

### 3.1 Ownership Summary

| Division | Member | Folders |
|---|---|---|
| **Hardprog** | HP-1 | `firmware/rocket_esp/src/{imu,orientation,csv_logger,telemetry}/`, `firmware/bench/{imu_bench,sd_bench}/` |
| | HP-2 | `firmware/rocket_esp/src/{comms,pyro,state}/`, `firmware/gcs_esp/`, `firmware/bench/{mosfet_dummy_load_bench,espnow_range_bench}/`, `hardware/` |
| | HP-1 + HP-2 | `firmware/rocket_esp/{platformio.ini,include/,src/main.cpp,src/utils/}`, `hardware/wiring/` |
| **Softprog** | SP-1 | `gcs/backend/`, `shared/protocol/` (with HP-1/HP-2) |
| | SP-2 | `gcs/frontend/` |
| | SP-3 | `.github/`, `README.md`, `docs/`, `data/`, `tools/`, `presentation/` |

**Rules for this structure:**
- `shared/protocol/` and `data/samples/` are the two integration points: change them only after announcing to the team, and update `docs/protocol.md` in the same PR.
- `rocket_esp` and `gcs_esp` each build alone (`pio run -d firmware/<name>`); both read packet definitions from `shared/protocol/`.
- `firmware/bench/` and `data/samples/` let the software side work **without hardware** and the hardware side test **without the dashboard**.
- No live ejection charge is ever connected for anything in `firmware/bench/`.
- Do not commit large logs, photos, or datasheets (keep files small; `.gitignore` covers `data/logs/`).

**Git rules:** one repo for the whole group, branch per feature (`feat/imu-yaw-pitch-roll`, `feat/dashboard-replay`), PR reviewed by at least 1 other member, `main` always builds. `shared/protocol/` is the HW ↔ SW contract; any change is announced to everyone. **All 5 members commit** (visible contribution). ⚠️ Confirm repo visibility / how the assessor gets access.

---

## 4. Bill of Materials

Source: team parts list, **estimated total Rp 233,500** (≈ Rp 21,227 per person, assuming 11 members per group).

| No | Item | Use | Qty | Unit price (Rp) | Total (Rp) |
|---|---|---|---|---|---|
| 1 | Triplek 3 mm | Bulkhead | 1 | 25,000 | 25,000 |
| 2 | Amplas | Grit 80, 320, 600, 1000 | 4 | 5,000 | 20,000 |
| 3 | Gunting / Cutter | Scissors 15k & cutter 10k | 1 | 25,000 | 25,000 |
| 4 | Baut | Fastener for the group | 50 | 200 | 10,000 |
| 5 | Lem G | Adhesive | 2 | 4,000 | 8,000 |
| 6 | PVC small diameter | Ejection charge holder (≈ ½ inch) / connection | 1 | 10,000 | 10,000 |
| 7 | Stik es krim | Resin applicator | 1 | 4,500 | 4,500 |
| 8 | PCB bolong (perfboard) | Small PCB ≈ 5×7 cm | 1 | 5,000 | 5,000 |
| 9 | MOSFET | Ejection charge switch | 1 | 5,000 | 5,000 |
| 10 | Resistor | Electronic component | 10 | 200 | 2,000 |
| 11 | Kapasitor | Electronic component | 10 | 200 | 2,000 |
| 12 | Dioda Schottky | Electronic component | 3 | 2,000 | 6,000 |
| 13 | Pin header | Electronic component | 6 | 1,500 | 9,000 |
| 14 | Jumper 10 cm | Electronic component | 40 | 400 | 16,000 |
| 15 | ESP32 DevKit V1 | Microcontroller | **1** | 86,000 | 86,000 |

> Items 1–5 and 7 are mostly structural/fabrication. Programmer-relevant items: **8–15**.

### 4.1 ⚠️ Gaps — Please Confirm

| # | Issue | Why it matters | Suggested action |
|---|---|---|---|
| 1 | **ESP count.** Team confirmed **2 ESP32** in total (Rocket ESP + ESP-Laptop); the organizer provides **1 ESP per team**, the rest is self-provided | The sheet lists only 1 | Buy **+1** ESP32 DevKit V1 (≈ Rp 86,000 at the listed price) |
| 2 | **IMU must give yaw/pitch/roll.** Model not chosen yet | MPU6050 (accel+gyro only) needs a filter (Madgwick/Mahony or DMP) and **yaw drifts** without a magnetometer. BNO055 outputs fused Euler angles directly | Confirm model; **BNO055** is the easiest path for stable yaw, MPU6050 is cheaper but needs more firmware work |
| 3 | **No microSD module** | On-board CSV logging is the safety net when the radio link drops (range ~1 km flight) | Add microSD module + card (now a must-have) |
| 4 | **No battery / regulator** | The Rocket ESP (and the ejection circuit) needs power | Confirm battery (1S/2S LiPo or 18650) + regulator + switch; the ESP-Laptop is powered by the laptop USB |
| 5 | **No igniter / e-match / nichrome** | The MOSFET needs a load | Confirm type and current draw (sizes MOSFET and battery) |
| 6 | **MOSFET part not specified** | Must be **logic-level** (3.3 V gate) and rated for the igniter current | Confirm part number (e.g., IRLZ44N or similar) |
| 7 | **No physical arming switch / jumper** | Hardware safety layer | Add |
| 8 | **No status LED / buzzer** | ARMED/READY feedback on the pad | Add |
| 9 | **Barometer — now optional** | KPI asks for yaw/pitch/roll only; a barometer is only needed if altitude is shown | Optional (BMP280/BME280); still recommended for altitude |
| 10 | **LoRa module — optional bonus** | Extra credit, not required | Only if time/budget allow |
| 11 | **Avionics bay dimensions unknown** | Determines board and battery size | Get from structure division |
| 12 | Soldering equipment, wire gauge for the ejection circuit, USB cables | Needed for build | Confirm who provides |

---

## 5. Team Structure & Job Descriptions

5 programmers: **2 Hardware Programmers (HP)** + **3 Software Programmers (SP)**. Names are placeholders.

### 5.1 Hardware Programmers

| Role | Member | Responsibilities |
|---|---|---|
| **HP-1 — Avionics module (ESP 2 task)** | ⚠️ _Name_ | • IMU driver and **yaw/pitch/roll** computation/calibration<br>• Avionics part of the Rocket ESP firmware: sampling, **CSV logging (SD)**, telemetry TX with seq + CRC<br>• Sensor wiring, perfboard soldering (avionics part)<br>• **Backup Pop Operator**<br>• Owner: `firmware/rocket_esp/src/{imu,orientation,csv_logger,telemetry}/`, `firmware/bench/{imu_bench,sd_bench}/` |
| **HP-2 — Ejection module (ESP 1 task), Comms & Power** | ⚠️ _Name_ | • Ejection part of the Rocket ESP firmware: command RX, ARM/DISARM/POP state machine, ACK, auto-SAFE<br>• Ejection circuit: logic-level MOSFET, gate pull-down, protection diode, safe default OFF<br>• ESP-NOW link between the 2 ESPs, ESP-Laptop (`gcs_esp`) firmware<br>• Power design, soldering (ejection part), wiring docs<br>• **Primary Pop Operator** (controls the ejection charge at the pop test)<br>• Owner: `firmware/rocket_esp/src/{comms,pyro,state}/`, `firmware/gcs_esp/`, `hardware/` |

### 5.2 Software Programmers

| Role | Member | Responsibilities |
|---|---|---|
| **SP-1 — GCS Backend & Protocol** | ⚠️ _Name_ | • Serial reader, packet parser, CRC check, packet-loss stats<br>• **CSV writer** and **CSV import/forward to dashboard**<br>• Command sender (ARM/DISARM/POP/PING) with ACK/timeout<br>• WebSocket server<br>• Maintains `shared/protocol/` + `docs/protocol.md` with HP-1/HP-2<br>• Owner: `gcs/backend/` |
| **SP-2 — Dashboard** | ⚠️ _Name_ | • Neat, complete dashboard: yaw/pitch/roll charts + orientation view, link status, node status, ejection state, event log<br>• **CSV replay mode** (worst-case path)<br>• ARM/DISARM/POP controls with 2-step confirmation<br>• Owner: `gcs/frontend/` |
| **SP-3 — Repo, Flowchart, Simulation & QA** | ⚠️ _Name_ | • Creates the **GitHub repo**, enforces structure, README, PR hygiene<br>• **Flowchart (PDF) + explanation** (see §6)<br>• `tools/fake_telemetry.py` simulator (needed Day 1)<br>• Test plan, integration testing, bug tracking, slides and demo script<br>• Owner: `.github/`, `README.md`, `docs/`, `data/`, `tools/`, `presentation/` |

### 5.3 Pairing Rules
- **HP-1 ↔ SP-1:** telemetry packet + CSV schema (Day 1).
- **HP-2 ↔ SP-1:** command/ACK protocol (Day 1).
- **SP-2 ↔ SP-3:** dashboard consumes simulated data and sample CSV until the real link works.
- **HP-1 ↔ HP-2:** both work in the single `firmware/rocket_esp` project — agree on module interfaces on Day 1, keep modules in separate folders to avoid merge conflicts, and never let one module block the other (see §12).
- **SP-3 ↔ HP-1/HP-2:** flowchart content reviewed by the module owners.
- Everyone helps SP-3 with testing on Days 6–7.

---

## 6. Flowchart (PDF) + Explanation — KPI Deliverable

**Format:** one PDF (`docs/flowchart/flowchart.pdf`) containing the flowcharts and a written explanation for each. Sources (draw.io / Mermaid) kept in `docs/flowchart/src/`. ⚠️ Confirm the language the assessor expects (Indonesian / English).

| # | Flowchart | Must show |
|---|---|---|
| 1 | **System overview** | Rocket nodes, ESP-Laptop, laptop backend, dashboard, data and command paths, CSV fallback |
| 2 | **Rocket ESP — avionics task (ESP 2)** | Boot → init IMU → loop: read IMU → compute yaw/pitch/roll → log CSV → send packet → repeat; IMU/SD error handling |
| 3 | **Rocket ESP — ejection task (ESP 1)** | Boot SAFE → wait command → validate (CRC, seq) → ARM → POP only if ARMED → fire pulse → ACK → auto-SAFE; timeout path; shown running alongside the avionics task (non-blocking) |
| 4 | **ESP-Laptop + backend** | Radio RX → serial → parse/validate → write CSV → WebSocket → dashboard; commands reverse path |
| 5 | **Dashboard + worst-case** | Live mode vs CSV replay mode, ejection control with confirmation |

**Explanation per flowchart:** purpose, inputs/outputs, step-by-step description, error/fallback handling, and which team member owns it.

---

## 7. Ejection Charge (Pop Test) — Operating Concept

The ejection charge is the **highest-risk** function and must not fire accidentally.

**Command sequence (proposed):**
1. Pop Operator clicks **ARM** → `CMD_ARM` → Rocket ESP replies `ACK_ARMED`.
2. Operator clicks **POP** → UI asks for a second confirmation (e.g., hold 2 s).
3. `CMD_POP` (seq + CRC) → Rocket ESP validates: must be ARMED, valid CRC, valid sequence.
4. Rocket ESP pulses the MOSFET for a fixed short time → replies `ACK_FIRED`.
5. Rocket ESP returns to **SAFE**. Link loss or invalid state → stay SAFE.

**Safety requirements (non-negotiable):**
- MOSFET gate **pull-down resistor** so the output is OFF during boot/reset.
- **Physical arming switch/jumper** in series with the ejection power path.
- Power-on default = **SAFE**; ARMED times out automatically.
- **No live charge on the bench**; develop and test with an LED/resistor as a dummy load.
- Live pop test only with the **team manager supervising**, everyone clear of the rocket, following the manager's safety procedure.
- Only the designated Pop Operator operates the live pop test.

---

## 8. Data & Protocol (Draft)

### 8.1 Avionics Packet (Rocket ESP → ESP-Laptop)
| Field | Type | Notes |
|---|---|---|
| `header` | uint8 | `0xAA` |
| `type` | uint8 | `0x01` = avionics |
| `seq` | uint16 | loss detection |
| `t_ms` | uint32 | uptime (ms) |
| `yaw, pitch, roll` | int16 ×3 | degrees × 100 |
| `ax, ay, az` | int16 ×3 | optional, scaled |
| `gx, gy, gz` | int16 ×3 | optional, scaled |
| `batt_mv` | uint16 | battery voltage |
| `flags` | uint8 | IMU OK, SD OK, etc. |
| `crc16` | uint16 | over previous bytes |

### 8.2 Command & Status (ESP-Laptop ↔ Rocket ESP)
| Packet | Direction | Fields |
|---|---|---|
| Command (`0xBB`) | ESP-Laptop → Rocket ESP | `cmd` (0x01 ARM, 0x02 DISARM, 0x03 POP, 0x04 PING), `seq`, `crc16` |
| Status/ACK (`0xCC`) | Rocket ESP → ESP-Laptop | `cmd`, `seq`, `state` (SAFE/ARMED/FIRED), `status`, `crc16` |

### 8.3 CSV Schema (storage + dashboard import)
```
timestamp_ms,seq,yaw_deg,pitch_deg,roll_deg,ax,ay,az,gx,gy,gz,batt_mv,flags
```
- One header row, one row per packet, UTF-8, `.` as decimal separator.
- Same schema for the on-board SD log (Rocket ESP) and the backend log, so **either file can be imported into the dashboard**.
- Ejection events are logged in a separate `events.csv` (`timestamp_ms,event,state`).

Final field lists are frozen on **Day 1** in `shared/protocol/packets.h` and `docs/protocol.md`.

---

## 9. Timeline

Today is **Thu 12 Nov**. Deadline **Wed 18 Nov**; flight and presentation **Thu 19 Nov**.

| Day | Date | Hardware Prog (HP-1, HP-2) | Software Prog (SP-1, SP-2, SP-3) | Milestone |
|---|---|---|---|---|
| **1** | Thu 12 Nov | Confirm gaps (§4.1), order/borrow +1 ESP + IMU + SD module; set up 2 PlatformIO projects (`rocket_esp`, `gcs_esp`) | **SP-3: create GitHub repo, invite all 5**; freeze packet + CSV schema v0.1; simulator; flowchart outline | ✅ Repo live, protocol v0.1 frozen, parts ordered |
| **2** | Fri 13 Nov | HP-1: IMU → yaw/pitch/roll on serial; HP-2: ESP-NOW hello between the 2 ESPs | SP-1: backend + **CSV writer** with simulator; SP-2: dashboard skeleton + **CSV import/replay**; SP-3: flowchart #1–#2 drafts | ✅ Orientation readable; CSV path started |
| **3** | Sat 14 Nov | HP-1: CSV logging (SD) + telemetry TX; HP-2: gcs_esp forwards to USB | SP-1: real packets → CSV → dashboard; SP-2: YPR charts + status | ✅ **Tier 1 data path: sensor → ESP-Laptop → CSV → dashboard** |
| **4** | Sun 15 Nov | HP-2: ejection module — command RX, ARM/POP logic, MOSFET with **LED dummy load**, ACK; then verify it runs alongside the avionics module without blocking | SP-1: command sender + ACK; SP-2: ARM/POP controls + confirmation; SP-3: flowchart #3–#5 drafts | ✅ **Tier 1 complete: ejection trigger from dashboard works (dummy load)** |
| **5** | Mon 16 Nov | Real-time stream tuning, range test, solder perfboard, power budget | Live dashboard polish (neat + complete), packet-loss stats, replay of recorded CSV | ✅ Tier 2: real-time + soldered boards on battery |
| **6** | Tue 17 Nov | Install in rocket (with structure team), handling/vibration test, battery runtime | Integration test, bug fixes, UI polish; **flowchart PDF final + review**; slides | ✅ **Feature freeze**, flowchart PDF done |
| **7** | Wed 18 Nov | Final check, spares, backup firmware; **supervised pop-test trial with manager (dummy load or per manager's procedure)** | Final README, repo tidy (tag release), sample CSV committed, rehearsal | ✅ **DEADLINE — all KPI deliverables complete** |
| **P** | Thu 19 Nov | Pre-flight checks; **HP-2 operates the pop test** under the manager's supervision | Operate GCS, record flight CSV, present | 🚀 **Flight, Pop Test & Presentation** |

**Cut line:** if parts or time slip, finish in this order: **(1) CSV data path → (2) ejection trigger + dashboard → (3) flowchart PDF → (4) real-time → (5) dashboard polish → (6) LoRa/extras.**

---

## 10. Deliverables (by 18 Nov)

**Tier 1 (mandatory)**
1. ✅ GitHub repository (one per group), all members contributing
2. ✅ Flowchart PDF + explanation
3. ✅ Avionics yaw/pitch/roll data into the ESP-Laptop, stored as CSV
4. ✅ Ejection charge trigger from the dashboard + dashboard visualization

**Tier 2 (plus points)**
5. ✅ Real-time data into the ESP-Laptop
6. ✅ Neat dashboard, complete display (charts, status, controls, replay, logs)

**Tier 3 (optional)**
7. ⬜ LoRa telemetry

**Supporting:** firmware for 2 boards (Rocket ESP, ESP-Laptop), `protocol.md`, `wiring.md`, `test-plan.md`, sample CSV logs, presentation slides + demo script.

---

## 11. Definition of Done

- CSV file with valid yaw/pitch/roll rows is produced from real sensor data and opens in the dashboard (import/replay).
- Dashboard button ARM → POP makes the Rocket ESP fire its output (dummy load in tests), ACK visible; firing is impossible when SAFE.
- (Plus) Live yaw/pitch/roll updates at ≥ 5 Hz with a link-lost indicator.
- (Plus) Dashboard layout is neat and shows: orientation, charts, link/node status, ejection state, event log, CSV replay.
- Flowchart PDF + explanation committed in `docs/flowchart/`.
- All code in `main`, builds from a clean clone, README up to date.
- System runs ≥ 30 min on battery ⚠️ (target to confirm).

---

## 12. Risks & Mitigation

| Risk | Impact | Mitigation |
|---|---|---|
| Only 1 ESP provided; extra ESP/IMU/SD parts arrive late | Hardware blocked | Order Day 1; software works on simulator + sample CSV |
| Real-time link drops during ~1 km flight | Lost telemetry | On-board SD CSV + dashboard replay (KPI-accepted worst case) |
| Yaw drift (IMU without magnetometer) | Inaccurate yaw | Prefer BNO055 or use DMP/Madgwick; calibrate; state the limitation in the presentation |
| Accidental ejection firing | **Safety incident** | Pull-down, hardware arm switch, SAFE default, dummy load for dev, manager supervision |
| Brownout when MOSFET fires | ESP reset | Decoupling capacitors, robust power path, test with dummy load |
| Protocol mismatch HW ↔ SW | Integration time lost | `shared/protocol/` single source of truth, freeze Day 1 |
| Both tasks (IMU/SD and ejection) share one Rocket ESP | A slow SD write or a hang could delay ejection handling | Non-blocking code or separate FreeRTOS tasks (ESP32 is dual-core), watchdog, ejection output stays OFF on reset/hang, test both running together by Day 4 |
| Time pressure | Missing KPI | Tier plan + cut line (§9), Tier 1 done by Sun 15 Nov |

---

## 13. Confirmed Decisions & Open Items

### 13.1 Confirmed
| # | Topic | Decision |
|---|---|---|
| 1 | Sensor | IMU (exact model TBD) |
| 2 | Framework | PlatformIO |
| 3 | Dates | 18 Nov = deadline; 19 Nov = flight day (+ presentation) |
| 4 | Expected altitude | ~1 km |
| 5 | Pop test | On 19 Nov, with a trial run beforehand; manager supervises |
| 6 | ESP count | **2 ESP32 in total** (Rocket ESP + ESP-Laptop); organizer provides 1, the team provides the other |
| 7 | KPI tasks | ESP 1 = fire ejection charge; ESP 2 = send yaw/pitch/roll (mapped to two modules on the Rocket ESP, see §13.2 #1) |
| 8 | LoRa | Optional bonus, not required |
| 9 | Pop operator | The programmer who controls the ejection charge |

### 13.2 Still open
1. ⚠️ **Task mapping:** with only 2 ESP32, both KPI tasks (ESP 1 = ejection, ESP 2 = yaw/pitch/roll) run on the Rocket ESP, and the second ESP32 is the ESP-Laptop. Confirm this satisfies the KPI.
2. ⚠️ KPI note "LoRa… bisa menggunakan pin handler" — my reading: LoRa is not required and a simpler link is acceptable. Please confirm what "pin handler" means (wired pin-header connection?).
3. ⚠️ IMU model (BNO055 vs MPU6050), SD module, barometer (optional).
4. ⚠️ Battery & igniter specification; avionics bay dimensions.
5. ⚠️ Is the 19 Nov pop test a **ground test** or **in-flight ejection** (e.g., at apogee)? This changes firmware logic and safety design.
6. ⚠️ Names of the 5 members and final role assignment; confirm Pop Operator (HP-2) and backup (HP-1).
7. ⚠️ GitHub repo visibility and how the assessor gets access; language of the flowchart PDF.
8. ⚠️ Date, place and procedure of the trial pop test (with the manager).
9. ⚠️ GCS stack (Python/FastAPI + web dashboard) is still an assumption — only PlatformIO is confirmed.