export class GcsAltitudeFilter {
  constructor({ timeConstantSeconds = 0.30, medianWindowSize = 5 } = {}) {
    this.timeConstantSeconds = timeConstantSeconds;
    this.medianWindowSize = medianWindowSize;
    this.rawSamples = [];
    this.filteredAltitude = null;
    this.previousTimeSeconds = null;
    this.previousWallTimeMs = null;
  }

  reset() {
    this.rawSamples = [];
    this.filteredAltitude = null;
    this.previousTimeSeconds = null;
    this.previousWallTimeMs = null;
  }

  update(rawAltitude, timestampUs = null) {
    if (!Number.isFinite(rawAltitude)) {
      return this.filteredAltitude;
    }

    this.rawSamples.push(rawAltitude);
    if (this.rawSamples.length > this.medianWindowSize) {
      this.rawSamples.shift();
    }

    const sortedSamples = [...this.rawSamples].sort((a, b) => a - b);
    const middle = Math.floor(sortedSamples.length / 2);
    const medianAltitude = sortedSamples.length % 2 === 1
      ? sortedSamples[middle]
      : (sortedSamples[middle - 1] + sortedSamples[middle]) / 2;

    let dt = 0.1;
    const currentTimeSeconds = Number.isFinite(timestampUs)
      ? timestampUs / 1_000_000
      : null;

    if (currentTimeSeconds !== null && this.previousTimeSeconds !== null) {
      const measuredDt = currentTimeSeconds - this.previousTimeSeconds;
      if (measuredDt > 0.001 && measuredDt < 2.0) {
        dt = measuredDt;
      }
    } else {
      const wallTimeMs = Date.now();
      if (this.previousWallTimeMs !== null) {
        const measuredDt = (wallTimeMs - this.previousWallTimeMs) / 1000;
        if (measuredDt > 0.001 && measuredDt < 2.0) {
          dt = measuredDt;
        }
      }
      this.previousWallTimeMs = wallTimeMs;
    }

    if (this.filteredAltitude === null) {
      this.filteredAltitude = medianAltitude;
    } else {
      // Complementary time-domain filter:
      // the barometer supplies the stable low-frequency altitude reference,
      // while previous estimates retain short-term flight continuity.
      const complementWeight = Math.exp(-dt / this.timeConstantSeconds);
      this.filteredAltitude =
        complementWeight * this.filteredAltitude
        + (1 - complementWeight) * medianAltitude;
    }

    if (currentTimeSeconds !== null) {
      this.previousTimeSeconds = currentTimeSeconds;
    }

    return this.filteredAltitude;
  }
}
