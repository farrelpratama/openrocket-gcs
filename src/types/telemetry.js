export function createEmptyTelemetry() {
  return {
    sequence: 0,
    timestampUs: 0,
    time: 0,

    ax: 0,
    ay: 0,
    az: 0,

    gx: 0,
    gy: 0,
    gz: 0,

    mpuTemp: 0,

    baroTemp: 0,

    pressurePa: 0,
    pressureHpa: 0,

    altitude: 0,

    roll: 0,
    pitch: 0,
    yaw: 0,

    rssi: null,
    batteryVoltage: 0, // <-- Ditambahkan agar widget baterai di Header bisa tampil

    status: "NO DATA",
  };
}