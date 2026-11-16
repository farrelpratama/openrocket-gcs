
import { createEmptyTelemetry } from "../../types/telemetry";

function parseCsvLine(line) {
  const values = [];

  let value = "";
  let quoted = false;

  for (let i = 0; i < line.length; i += 1) {
    const char = line[i];

    if (char === '"') {
      if (quoted && line[i + 1] === '"') {
        value += '"';
        i += 1;
      } else {
        quoted = !quoted;
      }
    } else if (char === "," && !quoted) {
      values.push(value.trim());
      value = "";
    } else {
      value += char;
    }
  }

  values.push(value.trim());

  return values;
}

const numeric = (value, fallback = 0) => {
  if (value === undefined || value === "") {
    return fallback;
  }

  const parsed = Number(value);

  return Number.isFinite(parsed)
    ? parsed
    : fallback;
};

export function parseTelemetryCSV(csvText) {
  const lines = csvText
    .split(/\r?\n/)
    .filter((line) => line.trim());

  if (lines.length < 2) {
    throw new Error(
      "CSV file contains no telemetry records."
    );
  }

  const headers = parseCsvLine(lines[0])
    .map((header) => header.toLowerCase());

  const rows = lines.slice(1).map(parseCsvLine);

  const currentFields = [
    "sequence",
    "timestamp_us",
    "ax",
    "ay",
    "az",
    "gx",
    "gy",
    "gz",
    "mpu_temp",
    "baro_temp",
    "pressure_pa",
    "pressure_hpa",
    "altitude",
    "roll",
    "pitch",
    "yaw",
  ];

  const isCurrentFormat = currentFields.every(
    (field) => headers.includes(field)
  );

  if (isCurrentFormat) {
    const index = Object.fromEntries(
      headers.map((header, i) => [header, i])
    );

    return rows.map((cols, rowIndex) => {
      const get = (key) => cols[index[key]];

      const timestampUs = numeric(
        get("timestamp_us"),
        rowIndex * 100000
      );

      return {
        ...createEmptyTelemetry(),

        sequence: numeric(get("sequence"), rowIndex),

        timestampUs,

        time: timestampUs / 1000000,

        ax: numeric(get("ax")),
        ay: numeric(get("ay")),
        az: numeric(get("az")),

        gx: numeric(get("gx")),
        gy: numeric(get("gy")),
        gz: numeric(get("gz")),

        mpuTemp: numeric(get("mpu_temp")),

        baroTemp: numeric(get("baro_temp")),

        pressurePa: numeric(get("pressure_pa")),

        pressureHpa: numeric(get("pressure_hpa")),

        altitude: numeric(get("altitude")),

        roll: numeric(get("roll")),
        pitch: numeric(get("pitch")),
        yaw: numeric(get("yaw")),

        status: "CSV REPLAY",
      };
    });
  }

  // Original dashboard CSV compatibility.
  const oldFields = [
    "mission_time",
    "yaw",
    "pitch",
    "roll",
    "altitude",
  ];

  if (
    oldFields.every((field) => headers.includes(field))
  ) {
    const index = Object.fromEntries(
      headers.map((header, i) => [header, i])
    );

    return rows.map((cols, rowIndex) => ({
      ...createEmptyTelemetry(),

      time: numeric(
        cols[index.mission_time],
        rowIndex * 0.1
      ),

      yaw: numeric(cols[index.yaw]),
      pitch: numeric(cols[index.pitch]),
      roll: numeric(cols[index.roll]),

      altitude: numeric(cols[index.altitude]),

      status: "LEGACY CSV REPLAY",
    }));
  }

  throw new Error(
    "Unsupported CSV format. Expected ESP32 telemetry CSV or legacy dashboard CSV."
  );
}
