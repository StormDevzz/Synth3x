use std::collections::HashMap;
use std::path::Path;

pub struct IconSet {
    pub map: HashMap<String, egui::TextureHandle>,
    pub dir: Option<egui::TextureHandle>,
    pub default: Option<egui::TextureHandle>,
}

impl IconSet {
    pub fn empty() -> Self {
        Self { map: HashMap::new(), dir: None, default: None }
    }

    pub fn for_file(&self, name: &str) -> Option<&egui::TextureHandle> {
        let ext = name.rsplit('.').next().unwrap_or("");
        self.map.get(ext).or(self.default.as_ref())
    }

    pub fn has_icons(&self) -> bool {
        !self.map.is_empty() || self.dir.is_some() || self.default.is_some()
    }
}

fn load_svg(ctx: &egui::Context, data: &[u8]) -> Option<egui::TextureHandle> {
    let img = egui_extras::image::load_svg_bytes(data).ok()?;
    Some(ctx.load_texture("icn", img, egui::TextureOptions::LINEAR))
}

pub fn load_all(ctx: &egui::Context) -> IconSet {
    let mut set = IconSet::empty();
    let dir = Path::new("assets/icons");
    if !dir.exists() { return set; }
    let rd = match std::fs::read_dir(dir) { Ok(d) => d, Err(_) => return set };
    for entry in rd.flatten() {
        let path = entry.path();
        let data = match std::fs::read(&path) { Ok(d) => d, Err(_) => continue };
        let stem = match path.file_stem().and_then(|n| n.to_str()) {
            Some(s) => s.to_owned(),
            None => continue,
        };
        let tex = match path.extension().and_then(|e| e.to_str()) {
            Some("svg") => load_svg(ctx, &data),
            _ => None,
        };
        let tex = match tex { Some(t) => t, None => continue };
        match stem.as_str() {
            "folder" => set.dir = Some(tex),
            "_" => set.default = Some(tex),
            ext => { set.map.insert(ext.to_owned(), tex); }
        }
    }
    set
}
