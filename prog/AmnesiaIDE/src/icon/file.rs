use crate::file::tree::FEntry;

pub fn for_entry(f: &FEntry) -> &'static str {
    if f.is_dir { "📁" } else { for_name(&f.name) }
}

pub fn for_name(name: &str) -> &'static str {
    let ext = name.rsplit('.').next().unwrap_or("");
    match ext {
        "rs" => "🦀",
        "c" | "h" => "⚡",
        "cpp" | "cc" | "cxx" | "hpp" | "hxx" => "⚡",
        "cs" => "🔷",
        "asm" | "s" | "S" => "⚙",
        "py" => "🐍",
        "js" | "ts" => "⬡",
        "html" | "htm" => "🌐",
        "css" | "scss" => "🎨",
        "md" | "txt" => "📄",
        "toml" | "json" | "yaml" | "yml" => "📋",
        "png" | "jpg" | "jpeg" | "gif" | "svg" => "🖼",
        "lock" | "gitignore" => "🔒",
        _ => "📄",
    }
}
