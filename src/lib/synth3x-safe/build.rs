use std::fs;
use std::path::Path;

fn main() {
    let version_path = Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../../VERSION");
    let version = fs::read_to_string(&version_path)
        .unwrap_or_else(|_| "0.8.1".to_string())
        .trim()
        .to_string();

    println!("cargo:rustc-env=SYNTH3X_VERSION={}", version);
    println!("cargo:rustc-env=SYNTH3X_VERSION_TAG={} Beta", version);
    println!("cargo:rerun-if-changed=../../../VERSION");
}
