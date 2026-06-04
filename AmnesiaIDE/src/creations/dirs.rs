pub fn home() -> String {
    dirs_fallback().unwrap_or_else(|| "/tmp/Amnesia".into())
}

fn dirs_fallback() -> Option<String> {
    if let Ok(h) = std::env::var("HOME") {
        Some(format!("{}/Amnesia", h))
    } else {
        None
    }
}

pub fn ensure_home() -> String {
    let p = home();
    let _ = std::fs::create_dir_all(&p);
    p
}

pub fn ensure_dir(path: &str) -> Result<(), String> {
    std::fs::create_dir_all(path).map_err(|e| e.to_string())
}
