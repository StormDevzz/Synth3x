pub fn read_file(path: &str) -> Result<String, String> {
    std::fs::read_to_string(path).map_err(|e| e.to_string())
}

pub fn write_file(path: &str, content: &str) -> Result<(), String> {
    std::fs::write(path, content).map_err(|e| e.to_string())
}

pub fn create_file(path: &str) -> Result<(), String> {
    if std::fs::metadata(path).is_ok() { return Err("Already exists".into()); }
    std::fs::write(path, "").map_err(|e| e.to_string())
}

#[allow(dead_code)]
pub fn create_dir(path: &str) -> Result<(), String> {
    std::fs::create_dir_all(path).map_err(|e| e.to_string())
}
