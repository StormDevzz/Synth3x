#[derive(Clone)]
pub struct FEntry {
    pub name: String,
    pub path: String,
    pub is_dir: bool,
    pub depth: u32,
}

pub fn scan_dir(path: &str, max_depth: u32) -> Vec<FEntry> {
    if max_depth > 4 { return vec![]; }
    let mut out = Vec::new();
    let dir = match std::fs::read_dir(path) { Ok(d) => d, Err(_) => return out };
    let mut items: Vec<_> = dir.flatten().collect();
    items.sort_by_key(|e| e.file_name());
    for e in items {
        let name = e.file_name().to_string_lossy().to_string();
        if name.starts_with('.') { continue; }
        let full = e.path().to_string_lossy().to_string();
        let is_dir = e.file_type().map(|t| t.is_dir()).ok().unwrap_or(false);
        out.push(FEntry { name: name.clone(), path: full.clone(), is_dir, depth: max_depth });
        if is_dir { out.extend(scan_dir(&full, max_depth + 1)); }
    }
    out
}
