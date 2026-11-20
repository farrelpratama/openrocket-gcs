use crate::modules::serial::write_serial_line;

#[tauri::command]
pub fn send_mission_command(command_type: String) -> Result<String, String> {
    match command_type.as_str() {
        "ARM" => {
            write_serial_line("STATUS")?;
            Ok("Status arm/check terkirim ke GCS ESP32".into())
        }
        "FIRE" | "EJECTION_POP" => {
            write_serial_line("FIRE")?;
            Ok("Perintah FIRE ejection charge terkirim ke GCS ESP32".into())
        }
        "LED_TEST_START" => {
            write_serial_line("TEST")?;
            Ok("Perintah TEST LED terkirim ke GCS ESP32".into())
        }
        "DISARM" | "OFF" | "ABORT" | "LED_TEST_ABORT" => {
            write_serial_line("OFF")?;
            Ok("Perintah OFF (Abort/Stop pemicu) terkirim ke GCS ESP32".into())
        }
        _ => Err(format!("Perintah tidak dikenali: {}", command_type)),
    }
}