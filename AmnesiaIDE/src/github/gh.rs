use std::process::Command;
use serde::Deserialize;

#[derive(Deserialize)]
struct GhResult {
    ok: bool,
    out: String,
    err: Option<String>,
    info: Option<String>,
}

pub fn run(args: &[&str]) -> String {
    let tool = find_gh();
    if tool.is_empty() {
        return "Error: gh tool not found. Build it: cd tools/gh && go build -o gh main.go".into();
    }
    let out = Command::new(&tool).args(args).output();
    match out {
        Ok(o) => {
            let stdout = String::from_utf8_lossy(&o.stdout).to_string();
            let stderr = String::from_utf8_lossy(&o.stderr).to_string();
            // Try to parse as JSON
            if let Ok(res) = serde_json::from_str::<GhResult>(&stdout) {
                let mut lines = Vec::new();
                if let Some(info) = res.info { lines.push(info); }
                if !res.out.is_empty() { lines.push(res.out); }
                if let Some(e) = res.err { lines.push(e); }
                format!("{}\n", lines.join("\n"))
            } else if !stdout.is_empty() {
                stdout
            } else {
                stderr
            }
        }
        Err(e) => format!("Error: {}\n", e),
    }
}

fn find_gh() -> String {
    // Check next to the binary first, then in tools/gh
    let candidates = vec![
        "tools/gh/gh".to_owned(),
        "tools/gh/gh.exe".to_owned(),
    ];
    for c in &candidates {
        if std::path::Path::new(c).exists() { return c.clone(); }
    }
    // Also check absolute from project root
    if let Ok(cwd) = std::env::current_dir() {
        let p = cwd.join("tools/gh/gh");
        if p.exists() { return p.to_string_lossy().to_string(); }
    }
    String::new()
}
