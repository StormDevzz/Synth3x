pub struct Compiler {
    pub name: &'static str,
    pub bin: &'static str,
    pub version_flag: &'static str,
    pub install_hint: &'static str,
}

pub const ALL: &[Compiler] = &[
    Compiler { name: "GCC (C)",  bin: "gcc",   version_flag: "--version", install_hint: "apt install gcc / pacman -S gcc" },
    Compiler { name: "G++ (C++)", bin: "g++",  version_flag: "--version", install_hint: "apt install g++ / pacman -S gcc" },
    Compiler { name: "Rustc",     bin: "rustc", version_flag: "--version", install_hint: "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh" },
    Compiler { name: "Mono (C#)", bin: "mcs",  version_flag: "--version", install_hint: "apt install mono-complete / pacman -S mono" },
    Compiler { name: "NASM",      bin: "nasm",  version_flag: "--version", install_hint: "apt install nasm / pacman -S nasm" },
];
