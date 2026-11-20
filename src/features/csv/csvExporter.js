
const CSV_COLUMNS = [
  ["sequence", "sequence"],
  ["timestamp_us", "timestampUs"],

  ["ax", "ax"],
  ["ay", "ay"],
  ["az", "az"],

  ["gx", "gx"],
  ["gy", "gy"],
  ["gz", "gz"],

  ["mpu_temp", "mpuTemp"],

  ["baro_temp", "baroTemp"],

  ["pressure_pa", "pressurePa"],
  ["pressure_hpa", "pressureHpa"],

  ["altitude", "altitude"],

  ["roll", "roll"],
  ["pitch", "pitch"],
  ["yaw", "yaw"],
];

export function exportTelemetryCSV(data) {
  const header = CSV_COLUMNS
    .map(([label]) => label)
    .join(",");

  const rows = data.map((point) =>
    CSV_COLUMNS
      .map(([, key]) => point[key] ?? "")
      .join(",")
  );

  return [header, ...rows].join("\n");
}

export function downloadTelemetryCSV(data) {
  if (!data.length) return false;

  const blob = new Blob(
    [exportTelemetryCSV(data)],
    { type: "text/csv;charset=utf-8" }
  );

  const url = URL.createObjectURL(blob);

  const anchor = document.createElement("a");

  anchor.href = url;

  anchor.download =
    `GMA_GCS_Telemetry_${Date.now()}.csv`;

  document.body.appendChild(anchor);

  anchor.click();

  anchor.remove();

  URL.revokeObjectURL(url);

  return true;
}
