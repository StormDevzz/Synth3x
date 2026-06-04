fn main() {
    cc::Build::new()
        .file("ccore/core.c")
        .include("ccore")
        .warnings(false)
        .compile("amnesia_core");
    println!("cargo:rerun-if-changed=ccore/core.c");
    println!("cargo:rerun-if-changed=ccore/core.h");
}
