
export function formatNumber(value, digits = 2) {
  return Number.isFinite(Number(value))
    ? Number(value).toFixed(digits)
    : "—";
}

export function formatMissionTime(seconds) {
  return `${formatNumber(seconds, 2)} s`;
}
