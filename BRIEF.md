# Project Brief — OpenRocket Telemetry, Ground Control Station & Pop Test System

> **Division:** Programmer (Hardware + Software)
> **Team size:** 5 members
> **Deadline:** 18 November 2026 (Wednesday)
> **Flight & Presentation Day:** 19 November 2026 (Thursday)
> **Status:** DRAFT v0.2 — items marked ⚠️ still need confirmation

---

## 1. Project Description

### 1.1 Background
Our aerospace team is building an OpenRocket-designed model rocket. The programmer division is responsible for the **electronics, firmware, data link, and ground software**: acquiring sensor data on the rocket, transmitting it wirelessly to the ground, visualizing it in real time, and remotely triggering the **pop test** (payload/parachute ejection test).

### 1.2 Objectives
1. **Data acquisition** — read sensors (IMU, barometer, etc.) on the rocket's ESP32 at a stable sample rate.
2. **Telemetry downlink** — send sensor data from the **Rocket ESP32** to the **GCS ESP32** wirelessly.
3. **GCS software** — receive data from the GCS ESP32 (USB serial) and visualize it **in real time** (charts, status, logs).
4. **Command uplink (Pop Test)** — the operator triggers a pop test from the GCS; the GCS ESP32 sends the command to the Rocket ESP32, which fires the ejection circuit (MOSFET switch → ejection charge) and releases the payload.
5. **Data logging** — store all telemetry to file for post-flight analysis and presentation.

### 1.3 Scope
| In scope | Out of scope |
|---|---|
| Rocket firmware (sensors, comms, pop-test logic) | Rocket airframe / structure (other divisions) |
| GCS ESP32 firmware (radio ↔ USB bridge) | Propulsion / motor selection |
| GCS desktop/web software (live dashboard, controls, logging) | Recovery mechanical design |
| Electronics wiring, soldering, power & ejection circuit | |
| Testing, documentation, presentation material | |

### 1.4 System Architecture

```
┌─────────────────────────────┐                        ┌──────────────────────────────┐
│          ROCKET             │                        │       GROUND CONTROL         │
│                             │   Telemetry (downlink) │                              │
│  Sensors ──► ESP32 Rocket ──┼───────────────────────►│ ESP32 GCS ──USB──► Laptop    │
│  (IMU, Baro, ...)           │                        │                    (GCS App) │
│            │                │◄───────────────────────┼──  Commands (uplink)         │
│            ▼                │   ARM / POP / PING     │                    ▲         │
│   MOSFET ► Ejection charge  │                        │                 Operator     │
└─────────────────────────────┘                        └──────────────────────────────┘
```

### 1.5 Technical Assumptions ⚠️ (confirm with team)
| Item | Proposed choice | Reason |
|---|---|---|
| ESP32 ↔ ESP32 link | **ESP-NOW** (2.4 GHz, no router) | Low latency, simple, supported natively on ESP32 DevKit V1 |
| Firmware framework | **PlatformIO** ✅ confirmed by team (Arduino framework) | Reproducible builds, good for team repo |
| GCS ↔ ESP32 GCS | USB Serial, 115200–921600 baud | Simplest, no extra hardware |
| GCS backend | **Python** (`pyserial` + `FastAPI`/WebSocket) | Fast to develop |
| GCS frontend | **Web dashboard** (HTML/JS + Chart.js/uPlot) | Easy live charts, easy to demo |
| Packet format | Fixed-size binary struct + CRC16 | Compact, fast, easy to validate |
| Telemetry rate | 10 Hz (target), 20 Hz (stretch) | Balance between resolution and link reliability |

> ⚠️ **Range risk:** the team's expected flight altitude is **~1 km**, so the link must cover ~1 km slant range. ESP-NOW with the DevKit V1 onboard PCB antenna typically gives only ~100–250 m in open air, and less with the ESP32 inside a rocket body. Options: (a) ESP-NOW long-range mode + external antenna (cheap, but unreliable at 1 km — must be range-tested), or (b) **LoRa modules** (e.g., SX1278/RFM95) on both ends, which comfortably reach ≥1 km LOS but at a lower data rate (expect ~2–5 Hz instead of 10 Hz; choose a frequency allowed in Indonesia). Decision pending — see §11.2.

---

## 2. Repository Structure

```
openrocket-gcs/
├── README.md
├── brief.md                      # this file
├── .gitignore
│
├── docs/
│   ├── architecture.md           # system diagram & data flow
│   ├── protocol.md               # packet formats, command list, ACK rules
│   ├── wiring.md                 # pinout & wiring tables
│   ├── test-plan.md              # test cases & results
│   └── images/                   # schematics, photos
│
├── shared/
│   └── protocol/
│       ├── packets.h             # C struct definitions (source of truth)
│       └── packets.py            # Python mirror of the structs
│
├── firmware/
│   ├── rocket/                   # PlatformIO project — Rocket ESP32
│   │   ├── platformio.ini
│   │   ├── include/
│   │   │   ├── config.h          # pins, rates, MAC addresses
│   │   │   └── packets.h         # (symlink/copy of shared)
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── sensors/          # imu.cpp, baro.cpp, (gps.cpp)
│   │   │   ├── comms/            # espnow_tx.cpp, command_rx.cpp
│   │   │   ├── pyro/             # ejection.cpp (arm/fire logic, MOSFET)
│   │   │   ├── state/            # state_machine.cpp
│   │   │   └── utils/            # crc16.cpp, logger.cpp
│   │   └── test/
│   │
│   └── gcs_bridge/               # PlatformIO project — GCS ESP32
│       ├── platformio.ini
│       └── src/
│           ├── main.cpp          # ESP-NOW RX → Serial, Serial → ESP-NOW TX
│           └── bridge.cpp
│
├── gcs/
│   ├── backend/
│   │   ├── requirements.txt
│   │   ├── main.py               # app entry
│   │   ├── serial_reader.py      # read & parse packets
│   │   ├── command_sender.py     # ARM / POP TEST / PING
│   │   ├── logger.py             # CSV/JSONL logging
│   │   └── ws_server.py          # WebSocket stream to frontend
│   └── frontend/
│       ├── index.html
│       ├── css/
│       └── js/
│           ├── charts.js         # altitude, accel, gyro, pressure
│           ├── status.js         # link status, battery, flight state
│           └── controls.js       # ARM / POP TEST buttons + confirmation
│
├── hardware/
│   ├── bom.md                    # parts list (from team list)
│   ├── schematics/               # circuit diagrams
│   └── pcb/                      # perfboard layout / notes
│
├── data/
│   ├── logs/                     # recorded telemetry (.csv)
│   └── samples/                  # fake data for GCS dev without hardware
│
├── tools/
│   ├── fake_telemetry.py         # simulator so software team isn't blocked
│   └── plot_log.py               # post-flight plotting
│
└── presentation/
    ├── slides/
    └── demo-script.md
```

**Git rules:** branch per feature (`feat/imu-driver`, `feat/gcs-dashboard`), PR reviewed by at least 1 other member, `main` always has a working build. `shared/protocol/` is the contract between hardware and software — changes must be announced to everyone.

---

## 3. Team Structure & Job Descriptions

The 5 programmers are split into **2 Hardware Programmers (HP)** and **3 Software Programmers (SP)**. Names are placeholders — fill in.

### 3.1 Hardware Programmers

| Role | Member | Responsibilities |
|---|---|---|
| **HP-1 — Rocket Sensing & Firmware Lead** | ⚠️ _Name_ | • Rocket ESP32 firmware architecture & flight state machine (IDLE → ARMED → ASCENT → APOGEE/EJECTION → DESCENT → LANDED)<br>• Sensor drivers (IMU, barometer, optional GPS) over I²C/SPI<br>• Sampling loop, filtering, altitude calculation<br>• Packet building (struct + CRC) and telemetry TX scheduling<br>• Wiring of sensor circuit, pin header/jumper assembly<br>• Owner of `firmware/rocket/` (sensors, state, utils) |
| **HP-2 — Comms, Pyro & Power** | ⚠️ _Name_ | • ESP-NOW setup (pairing, MAC config, retries, link-loss handling)<br>• Command RX on rocket + `firmware/gcs_bridge/` (GCS ESP32)<br>• Pop-test ejection circuit: MOSFET driver, gate resistor, flyback/protection, safe default OFF state<br>• Arm/Disarm/Fire logic with safety interlocks and ACK<br>• Power design (battery, regulation, switch, power budget)<br>• Soldering PCB / perfboard, wiring documentation<br>• Owner of `firmware/gcs_bridge/`, `firmware/rocket/{comms,pyro}`, `hardware/` |

### 3.2 Software Programmers

| Role | Member | Responsibilities |
|---|---|---|
| **SP-1 — GCS Backend & Protocol** | ⚠️ _Name_ | • Serial reader, packet parser, CRC validation, packet-loss statistics<br>• Command sender (ARM, DISARM, POP TEST, PING) with ACK/timeout handling<br>• WebSocket server feeding the frontend<br>• Maintains `shared/protocol/` + `docs/protocol.md` together with HP-2<br>• Owner of `gcs/backend/` (except logger) |
| **SP-2 — GCS Frontend / Dashboard** | ⚠️ _Name_ | • Real-time dashboard: altitude, acceleration, gyro, pressure, temperature, battery, link quality, flight state<br>• Control panel UI: ARM/DISARM toggle, POP TEST with 2-step confirmation, command status feedback<br>• Clean, readable layout for presentation (dark theme, large numbers)<br>• Owner of `gcs/frontend/` |
| **SP-3 — Data Logging, Simulation & QA** | ⚠️ _Name_ | • Logger (CSV/JSONL with timestamps), log replay<br>• `tools/fake_telemetry.py` simulator so SP-1/SP-2 can work before hardware is ready (**needed on Day 1**)<br>• Post-flight plotting script<br>• Test plan execution, bug tracking, integration testing<br>• Documentation, README, presentation slides & demo script<br>• Owner of `data/`, `tools/`, `docs/test-plan.md`, `presentation/` |

### 3.3 Pairing Rules
- **HP-1 ↔ SP-1:** agree on telemetry packet layout (Day 1).
- **HP-2 ↔ SP-1:** agree on command/ACK protocol (Day 1).
- **SP-2 ↔ SP-3:** dashboard consumes simulated data until real link works.
- Everyone helps SP-3 with testing on Days 6–7.

---

## 4. Bill of Materials (from team list)

Source: team parts list, **estimated total Rp 233,500** (≈ Rp 21,227 per person, assumption 11 members per group).

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

> Items 1–5, 7 are mostly structural/fabrication (other divisions may share them). Programmer-relevant items: **8–15**.

### 4.1 ⚠️ Gaps Found in the Parts List — Please Confirm

| # | Issue | Why it matters | Suggested action |
|---|---|---|---|
| 1 | **Only 1 ESP32 listed** | System needs **2** (Rocket + GCS) — team confirmed 2 will be used | Update the sheet / buy +1 ESP32 DevKit V1 (≈ Rp 86,000) |
| 2 | **No sensors listed** | Team confirmed **IMU**. IMU-only altitude drifts heavily (double integration of acceleration) — not reliable at ~1 km | Confirm IMU model (MPU6050 / BNO055); **strongly recommend adding a barometer (BMP280 / BME280)**; GPS optional |
| 3 | **No battery / power source** | Rocket ESP32 and ejection circuit need power | Confirm battery (e.g., 1S/2S LiPo or 18650) + regulator/BEC + power switch + charger |
| 4 | **No igniter / e-match / nichrome** | The MOSFET needs a load to drive | Confirm igniter type and its current draw (determines MOSFET & battery choice) |
| 5 | **MOSFET type not specified** | Must be **logic-level** (3.3 V gate) and rated for the igniter current | Confirm part number (e.g., IRLZ44N / similar) |
| 6 | **No physical arming switch / safety plug** | Needed as a hardware safety layer | Add a removable arming jumper/switch |
| 7 | **No status indicator** (LED/buzzer) | Needed for ARMED/READY feedback on the pad | Add LED + buzzer (cheap) |
| 8 | **No data backup storage** (e.g., microSD module) | Wireless link can drop; onboard log is a backup | Optional but recommended |
| 9 | **No USB cables / antenna decision** | GCS needs USB; range may need antenna | Confirm cables; confirm required range |
| 10 | **Rocket dimensions unknown** (body diameter, avionics bay length) | Determines board size and battery fit | Get dimensions from structure division |
| 11 | **Soldering equipment** (iron, solder, wire) not listed | Needed for perfboard | Confirm who provides it |
| 12 | **Wire for ejection circuit** (gauge/length) not listed | Current-carrying wire must be adequate | Confirm |

---

## 5. Pop Test — Operating Concept

The pop test is the **highest-risk** function. It must be deliberately designed to prevent accidental firing.

**Command sequence (proposed):**
1. Operator clicks **ARM** in GCS → GCS sends `CMD_ARM` → rocket replies `ACK_ARMED` (dashboard shows ARMED).
2. Operator clicks **POP TEST** → UI asks for second confirmation (e.g., hold button 2 s / type confirm).
3. GCS sends `CMD_POP` (with sequence number + CRC) → rocket validates: must be ARMED, valid CRC, valid sequence.
4. Rocket pulses the MOSFET for a fixed, short duration → replies `ACK_FIRED`.
5. Rocket auto-returns to **SAFE** after the pulse. Any link loss or invalid state → stay SAFE.

**Safety requirements (non-negotiable):**
- MOSFET gate has a **pull-down resistor** so the output is OFF during boot/reset/ESP32 floating pins.
- Physical **arming switch/jumper** in series with the ejection power path (defense in depth).
- Default state at power-on = **SAFE**. ARMED state times out automatically if no command is received.
- **Pop test is never performed with a live charge during bench development** — use an LED/resistor as a dummy load until the final supervised test.
- The live test is done only with a **supervisor/senior present**, people clear of the rocket, and following the team's safety procedure.

---

## 6. Telemetry & Command Protocol (Draft)

### 6.1 Telemetry Packet (Rocket → GCS)
| Field | Type | Notes |
|---|---|---|
| `header` | uint8 | `0xAA` |
| `type` | uint8 | `0x01` = telemetry |
| `seq` | uint16 | packet counter (loss detection) |
| `t_ms` | uint32 | rocket uptime (ms) |
| `state` | uint8 | SAFE / ARMED / ASCENT / EJECTED / DESCENT / LANDED |
| `ax, ay, az` | int16 ×3 | accel (scaled) |
| `gx, gy, gz` | int16 ×3 | gyro (scaled) |
| `pressure` | uint32 | Pa |
| `altitude` | int16 | dm or cm (agree) |
| `temp` | int16 | 0.01 °C |
| `batt_mv` | uint16 | battery voltage |
| `flags` | uint8 | sensor OK bits, pyro armed bit |
| `crc16` | uint16 | over all previous bytes |

### 6.2 Command Packet (GCS → Rocket)
| Field | Type | Notes |
|---|---|---|
| `header` | uint8 | `0xBB` |
| `cmd` | uint8 | `0x01` ARM, `0x02` DISARM, `0x03` POP, `0x04` PING |
| `seq` | uint16 | command counter |
| `crc16` | uint16 | |

Rocket replies with an `ACK` packet (`cmd`, `seq`, `status`). Final field lists/sizes are frozen on **Day 1** in `shared/protocol/packets.h`.

---

## 7. Timeline

Today is **Thu 12 Nov**. Total working window: **7 days** until deadline (18 Nov), presentation on 19 Nov.

| Day | Date | Hardware Prog (HP-1, HP-2) | Software Prog (SP-1, SP-2, SP-3) | Milestone |
|---|---|---|---|---|
| **1** | Thu 12 Nov | Confirm BOM gaps (§4.1), order/borrow missing parts; HP-1: set up rocket PlatformIO project; HP-2: set up gcs_bridge project | SP-1+HP: freeze packet draft; SP-3: repo + fake telemetry simulator; SP-2: dashboard skeleton | ✅ Repo ready, protocol v0.1 frozen, parts ordered |
| **2** | Fri 13 Nov | HP-1: read IMU & barometer on breadboard; HP-2: ESP-NOW "hello" between 2 ESP32 | SP-1: serial reader + parser with simulator; SP-2: first live charts (simulated data) | ✅ Sensors read; ESP-NOW link works; GCS shows fake data |
| **3** | Sat 14 Nov | HP-1: telemetry packet + TX loop; HP-2: GCS bridge forwards packets to USB serial | SP-1: real packets into backend; SP-2: status panel (link, battery, state); SP-3: logger | ✅ **End-to-end telemetry: sensor → rocket ESP32 → GCS ESP32 → dashboard** |
| **4** | Sun 15 Nov | HP-2: MOSFET circuit on breadboard with **LED dummy load**, arm/pop logic + ACK; HP-1: flight state machine, filtering | SP-1: command sender + ACK/timeout; SP-2: ARM/POP controls with confirmation | ✅ **Pop test command works end-to-end (dummy load)** |
| **5** | Mon 16 Nov | Solder perfboard (power, sensors, MOSFET); power budget check; range test (open area) | Packet-loss stats, link-lost warning, log replay; SP-3: test plan execution #1 | ✅ Soldered avionics board running on battery |
| **6** | Tue 17 Nov | Install in rocket avionics bay (with structure team), vibration/handling test, battery runtime test | Full integration test, bug fixing, UI polish; SP-3: slides + demo script | ✅ **Feature freeze**; full system test passed |
| **7** | Wed 18 Nov | Final check, spare parts, backup firmware flashed; **supervised pop-test trial with manager (dummy load or per manager's procedure)** | Final docs/README, rehearsal, backup laptop with software installed | ✅ **DEADLINE — deliverables complete** |
| **P** | Thu 19 Nov | Pre-flight checks, supervised pop test, operate hardware | Operate GCS, record flight log, present | 🚀 **Flight, Pop Test & Presentation Day** |

**Buffer:** If parts arrive late (sensors/ESP32), Days 2–4 continue with the simulator for software, and HP team prioritizes in this order: **(1) telemetry link → (2) pop test → (3) extras (GPS, SD logging)**.

---

## 8. Deliverables (by 18 Nov)

1. ✅ Rocket firmware (sensors + telemetry + pop test)
2. ✅ GCS ESP32 bridge firmware
3. ✅ GCS software with real-time dashboard, controls, and logging
4. ✅ Working avionics board installed in rocket
5. ✅ Documentation: `protocol.md`, `wiring.md`, `test-plan.md`, README
6. ✅ Recorded sample telemetry log + plots
7. ✅ Presentation slides + demo script

---

## 9. Definition of Done

- GCS shows live altitude, accel, gyro, pressure, battery, flight state at ≥ 10 Hz with link-lost indication.
- Telemetry link works at the required range (~1 km target) with packet loss < 5%, verified by a range test ⚠️ (depends on radio decision).
- Pop test command: ARM → POP → rocket fires output + ACK received on GCS; no firing possible when SAFE.
- System runs ≥ 30 minutes on battery ⚠️ (target to confirm).
- All code in `main`, builds from clean clone, documented.

---

## 10. Risks & Mitigation

| Risk | Impact | Mitigation |
|---|---|---|
| Missing sensors/ESP32 not arriving in time | Blocks hardware work | Order Day 1; software continues with simulator |
| ESP-NOW range insufficient | Telemetry lost in flight | Range test Day 5; fallback: external antenna / LoRa (needs extra parts) |
| Accidental firing of ejection output | **Safety incident** | Pull-down, hardware arm switch, SAFE default, dummy load for development |
| Brownout when MOSFET fires | ESP32 resets | Decoupling capacitors, separate/robust power path, test with dummy load |
| Protocol mismatch HW ↔ SW | Wasted integration time | Single source of truth `shared/protocol/`, freeze Day 1 |
| Time pressure | Incomplete features | Strict priority order, feature freeze Day 6 |

---

## 11. Confirmed Decisions & Open Items

### 11.1 Confirmed by the team
| # | Topic | Decision |
|---|---|---|
| 1 | Sensors | IMU (exact model TBD) |
| 2 | ESP32 | 2 units (Rocket + GCS); parts sheet lists only 1 and must be updated |
| 3 | Firmware framework | PlatformIO |
| 4 | Dates | 18 Nov = deadline; 19 Nov = flight day (+ presentation) |
| 5 | Expected flight altitude | ~1 km |
| 6 | Pop test | Performed on 19 Nov; trial run beforehand for troubleshooting |
| 7 | Safety supervision | Team manager supervises the pop test |

### 11.2 Still open
1. ⚠️ **Radio link for ~1 km** — ESP-NOW (+ long-range mode / external antenna) vs. LoRa modules (§1.5).
2. ⚠️ **Barometer** — add BMP280/BME280 for reliable altitude (§4.1 #2)?
3. ⚠️ Battery & igniter specification (pending from team).
4. ⚠️ Avionics bay dimensions (pending from structure division).
5. ⚠️ Names of the 5 members and role assignment (§3).
6. ⚠️ Is the 19 Nov pop test a **ground test** (before/after flight) or **in-flight ejection** (e.g., at apogee)? This changes the firmware logic and the safety design.
7. ⚠️ Date, place, and procedure for the trial pop test (set together with the manager).
8. ⚠️ GCS stack (Python/FastAPI + web dashboard) is still an assumption — only PlatformIO has been confirmed.
