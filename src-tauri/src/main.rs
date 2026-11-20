// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod modules;

use modules::commands::send_mission_command;
use modules::serial::{list_serial_ports, start_serial_listener, stop_serial_listener};
use modules::logger::save_telemetry_to_csv; // <-- Pastikan ini diimpor

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![
            list_serial_ports,
            start_serial_listener,
            stop_serial_listener,
            send_mission_command,
            save_telemetry_to_csv // <-- Pastikan terdaftar di sini
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}