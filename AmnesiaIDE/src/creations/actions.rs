use crate::creations::dirs;
use crate::creations::templates;

pub fn new_file(path: &str) -> Result<String, String> {
    if std::fs::metadata(path).is_ok() { return Err("Already exists".into()); }
    let ext = path.rsplit('.').next().unwrap_or("");
    let content = templates::template(ext);
    std::fs::write(path, content).map_err(|e| e.to_string())?;
    Ok(content.to_owned())
}

pub fn delete_file(path: &str) -> Result<(), String> {
    let m = std::fs::metadata(path).map_err(|e| e.to_string())?;
    if m.is_dir() {
        std::fs::remove_dir_all(path).map_err(|e| e.to_string())
    } else {
        std::fs::remove_file(path).map_err(|e| e.to_string())
    }
}

pub fn rename_file(old: &str, new: &str) -> Result<(), String> {
    std::fs::rename(old, new).map_err(|e| e.to_string())
}

pub fn new_dir(path: &str) -> Result<(), String> {
    dirs::ensure_dir(path)
}
