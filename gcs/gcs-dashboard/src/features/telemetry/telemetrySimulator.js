
export function generateSimulatedTelemetry(elapsed) {
  let altitude = 0;
  let status = "STANDBY / PAD";

  if (elapsed >= 2 && elapsed < 7) {
    status = "POWERED FLIGHT (BOOST)";

    altitude = 45 * Math.pow(elapsed - 2, 1.8);
  } else if (elapsed >= 7 && elapsed < 12) {
    status = "COASTING TO APOGEE";

    altitude = 815 - 4.5 * Math.pow(elapsed - 7, 2);
  } else if (elapsed >= 12) {
    status = "EJECTION & DESCENT";

    altitude = Math.max(
      0,
      702 - 14 * (elapsed - 12)
    );
  }

  return {
    sequence: Math.round(elapsed * 10),

    timestampUs: Math.round(elapsed * 1000000),

    time: elapsed,

    // MPU6050
    ax: 0,
    ay: 0,
    az: 9.80665,

    gx: 0,
    gy: 0,
    gz: 0,

    mpuTemp: 25,

    // BMP180
    baroTemp: 25,
    pressurePa: 101325,
    pressureHpa: 1013.25,

    altitude: Number(altitude.toFixed(2)),

    // Orientation
    yaw: Number(
      (Math.sin(elapsed * 0.6) * 12).toFixed(2)
    ),

    pitch: Number(
      (Math.sin(elapsed * 1.1) * 18).toFixed(2)
    ),

    roll: Number(
      (Math.cos(elapsed * 0.9) * 10).toFixed(2)
    ),

    rssi: null,
    status,
  };
}
