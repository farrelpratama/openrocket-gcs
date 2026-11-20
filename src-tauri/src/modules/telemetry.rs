use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct TelemetryData {
    pub sequence: u64,
    pub timestamp_us: u64,
    pub time: f64,

    pub ax: f64,
    pub ay: f64,
    pub az: f64,

    pub gx: f64,
    pub gy: f64,
    pub gz: f64,

    pub mpu_temp: f64,
    pub baro_temp: f64,
    pub pressure_pa: f64,
    pub pressure_hpa: f64,
    pub altitude: f64,

    pub roll: f64,
    pub pitch: f64,
    pub yaw: f64,

    pub rssi: Option<i32>,
    pub battery_voltage: Option<f64>,
    pub status: String,
}

impl TelemetryData {
    /// Parsing 1 baris CSV dari gcs_esp:
    /// sequence,timestamp_us,ax,ay,az,gx,gy,gz,mpu_temp,baro_temp,pressure_pa,pressure_hpa,altitude,roll,pitch,yaw
    pub fn from_gcs_csv(line: &str) -> Option<Self> {
        let trimmed = line.trim();
        if trimmed.is_empty() || trimmed.starts_with('#') || trimmed.starts_with("sequence,") {
            return None;
        }

        let parts: Vec<&str> = trimmed.split(',').map(|s| s.trim()).collect();
        if parts.len() < 16 {
            return None;
        }

        let sequence: u64 = parts[0].parse().ok()?;
        let timestamp_us: u64 = parts[1].parse().ok()?;
        let time = (timestamp_us as f64) / 1_000_000.0;

        Some(Self {
            sequence,
            timestamp_us,
            time,
            ax: parts[2].parse().ok()?,
            ay: parts[3].parse().ok()?,
            az: parts[4].parse().ok()?,
            gx: parts[5].parse().ok()?,
            gy: parts[6].parse().ok()?,
            gz: parts[7].parse().ok()?,
            mpu_temp: parts[8].parse().ok()?,
            baro_temp: parts[9].parse().ok()?,
            pressure_pa: parts[10].parse().ok()?,
            pressure_hpa: parts[11].parse().ok()?,
            altitude: parts[12].parse().ok()?,
            roll: parts[13].parse().ok()?,
            pitch: parts[14].parse().ok()?,
            yaw: parts[15].parse().ok()?,
            rssi: None,
            battery_voltage: None,
            status: "LIVE".to_string(),
        })
    }
}