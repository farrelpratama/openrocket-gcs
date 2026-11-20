use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::PathBuf;
use tauri::AppHandle;
use tauri::Manager;

pub fn get_log_file_path(app_handle: &AppHandle) -> Result<PathBuf, String> {
    let app_dir = app_handle
        .path()
        .document_dir()
        .map_err(|e| e.to_string())?
        .join("GCS_Rocket_Logs");

    fs::create_dir_all(&app_dir).map_err(|e| e.to_string())?;

    let timestamp = chrono::Local::now().format("%Y-%m-%d_%H-%M-%S");
    let filename = format!("flight_log_{}.csv", timestamp);
    
    Ok(app_dir.join(filename))
}

// Wajib ada atribut ini agar dikenali oleh tauri::generate_handler!
#[tauri::command]
pub fn save_telemetry_to_csv(app_handle: AppHandle, data_json: String) -> Result<String, String> {
    let file_path = get_log_file_path(&app_handle)?;
    
    let file_exists = file_path.exists();
    let mut file = OpenOptions::new()
        .create(true)
        .append(true)
        .open(&file_path)
        .map_err(|e| e.to_string())?;

    if !file_exists {
        writeln!(file, "timestamp,altitude,velocity,acceleration,status")
            .map_err(|e| e.to_string())?;
    }

    writeln!(file, "{}", data_json).map_err(|e| e.to_string())?;

    Ok(file_path.to_string_lossy().into_owned())
}