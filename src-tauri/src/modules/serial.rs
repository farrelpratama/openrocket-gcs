use serialport::SerialPort;
use std::io::{BufRead, BufReader, Write};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, OnceLock};
use std::thread;
use std::time::Duration;
use tauri::{AppHandle, Emitter};

struct SerialSession {
    running: Arc<AtomicBool>,
    writer: Arc<Mutex<Option<Box<dyn SerialPort + Send>>>>,
}

static SESSION: OnceLock<Mutex<Option<SerialSession>>> = OnceLock::new();

fn session_store() -> &'static Mutex<Option<SerialSession>> {
    SESSION.get_or_init(|| Mutex::new(None))
}

#[tauri::command]
pub fn list_serial_ports() -> Result<Vec<String>, String> {
    match serialport::available_ports() {
        Ok(ports) => Ok(ports.into_iter().map(|p| p.port_name).collect()),
        Err(e) => Err(format!("Gagal memindai port serial: {}", e)),
    }
}

#[tauri::command]
pub fn start_serial_listener(
    app: AppHandle,
    port_name: String,
    baud_rate: u32,
) -> Result<String, String> {
    // Hentikan sesi lama jika masih aktif agar bisa reconnect mulus
    let _ = stop_serial_listener();

    let port = serialport::new(&port_name, baud_rate)
        .timeout(Duration::from_millis(100))
        .open()
        .map_err(|e| format!("Gagal membuka port {}: {}", port_name, e))?;

    // Salin handle untuk penulisan perintah serial (write)
    let port_for_write = port
        .try_clone()
        .map_err(|e| format!("Gagal cloning port serial: {}", e))?;

    let running = Arc::new(AtomicBool::new(true));
    let running_thread = Arc::clone(&running);

    let writer_arc: Arc<Mutex<Option<Box<dyn SerialPort + Send>>>> =
        Arc::new(Mutex::new(Some(port_for_write)));

    {
        let mut guard = session_store().lock().map_err(|e| e.to_string())?;
        *guard = Some(SerialSession {
            running: Arc::clone(&running),
            writer: Arc::clone(&writer_arc),
        });
    }

    // Bersihkan buffer lama agar langsung menangkap data terbaru
    let _ = port.clear(serialport::ClearBuffer::All);

    let _ = app.emit(
        "serial-log",
        format!("# Serial listener started on {} at {} baud", port_name, baud_rate),
    );

    thread::spawn(move || {
        let mut reader = BufReader::new(port);
        let mut line = String::new();

        while running_thread.load(Ordering::SeqCst) {
            line.clear();
            match reader.read_line(&mut line) {
                Ok(0) => {
                    thread::sleep(Duration::from_millis(10));
                }
                Ok(_) => {
                    let trimmed = line.trim_end_matches(&['\r', '\n'][..]);
                    if trimmed.is_empty() {
                        continue;
                    }

                    let _ = app.emit("serial-line", trimmed.to_string());

                    if trimmed.starts_with('#') || trimmed.starts_with("[COMMAND]") {
                        let _ = app.emit("serial-log", trimmed.to_string());
                    }
                }
                Err(ref e) if e.kind() == std::io::ErrorKind::TimedOut => {
                    // Timeout normal saat belum ada baris baru; lanjut loop real-time
                    continue;
                }
                Err(e) => {
                    let _ = app.emit(
                        "serial-log",
                        format!("# [Serial Read Error] {}", e),
                    );
                    running_thread.store(false, Ordering::SeqCst);
                    break;
                }
            }
        }
    });

    Ok(format!("Terhubung ke {} ({} baud)", port_name, baud_rate))
}

#[tauri::command]
pub fn stop_serial_listener() -> Result<String, String> {
    let mut guard = session_store().lock().map_err(|e| e.to_string())?;
    if let Some(session) = guard.take() {
        session.running.store(false, Ordering::SeqCst);
        if let Ok(mut w) = session.writer.lock() {
            *w = None;
        }
    }
    Ok("Koneksi serial diputus.".into())
}

pub fn write_serial_line(payload: &str) -> Result<(), String> {
    let guard = session_store().lock().map_err(|e| e.to_string())?;
    let session = guard
        .as_ref()
        .ok_or_else(|| "Port serial belum terhubung.".to_string())?;

    if !session.running.load(Ordering::SeqCst) {
        return Err("Listener serial tidak aktif.".to_string());
    }

    let mut writer_guard = session.writer.lock().map_err(|e| e.to_string())?;
    let port = writer_guard
        .as_mut()
        .ok_or_else(|| "Handle write serial tidak tersedia.".to_string())?;

    let message = format!("{}\n", payload.trim());
    port.write_all(message.as_bytes())
        .map_err(|e| format!("Gagal menulis ke serial: {}", e))?;
    port.flush()
        .map_err(|e| format!("Gagal flush serial: {}", e))?;

    Ok(())
}