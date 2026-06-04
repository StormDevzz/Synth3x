use super::Lang;

fn w(path: &str, content: &str) {
    let _ = std::fs::write(path, content);
}

fn source(lang: Lang) -> &'static str {
    use Lang::*;
    match lang {
        C => r#"#include <stdio.h>
int main(void) {
    printf("Hello from AmnesiaIDE!\n");
    return 0;
}
"#,
        Cpp => r#"#include <iostream>
int main() {
    std::cout << "Hello from AmnesiaIDE!" << std::endl;
    return 0;
}
"#,
        Csharp => r#"using System;
class Program {
    static void Main() {
        Console.WriteLine("Hello from AmnesiaIDE!");
    }
}
"#,
        Rust => r#"fn main() {
    println!("Hello from AmnesiaIDE!");
}
"#,
        Go => r#"package main
import "fmt"
func main() {
    fmt.Println("Hello from AmnesiaIDE!")
}
"#,
        Zig => r#"const std = @import("std");
pub fn main() !void {
    const stdout = std.io.getStdOut().writer();
    try stdout.print("Hello from AmnesiaIDE!\n", .{});
}
"#,
        Nim => r#"echo "Hello from AmnesiaIDE!"
"#,
        Asm => r#"global _start
section .text
_start:
    mov rax, 1
    mov rdi, 1
    mov rsi, msg
    mov rdx, len
    syscall
    mov rax, 60
    xor rdi, rdi
    syscall
section .data
msg: db "Hello from AmnesiaIDE!", 10
len: equ $ - msg
"#,
        Python => r#"print("Hello from AmnesiaIDE!")
"#,
        JavaScript => r#"console.log("Hello from AmnesiaIDE!");
"#,
        TypeScript => r#"const greet: string = "Hello from AmnesiaIDE!";
console.log(greet);
"#,
        Lua => r#"print("Hello from AmnesiaIDE!")
"#,
        OCaml => r#"print_endline "Hello from AmnesiaIDE!";;
"#,
        Haskell => r#"main = putStrLn "Hello from AmnesiaIDE!"
"#,
    }
}

fn howto(lang: Lang, name: &str) -> String {
    use Lang::*;
    match lang {
        C => format!("gcc -Wall -Wextra -o {name} src/main.c && ./{name}"),
        Cpp => format!("g++ -Wall -Wextra -std=c++17 -o {name} src/main.cpp && ./{name}"),
        Csharp => format!("mcs src/main.cs -out:{name}.exe && mono {name}.exe"),
        Rust => format!("rustc -o {name} src/main.rs && ./{name}"),
        Go => format!("go build -o {name} src/main.go && ./{name}"),
        Zig => format!("zig build-exe src/main.zig -o {name} && ./{name}"),
        Nim => format!("nim compile --run src/main.nim"),
        Asm => format!("nasm -f elf64 src/main.asm -o main.o && ld -o {name} main.o && ./{name}"),
        Python => format!("python3 src/main.py"),
        JavaScript => format!("node src/main.js"),
        TypeScript => format!("npx tsc src/main.ts && node src/main.js"),
        Lua => format!("lua src/main.lua"),
        OCaml => format!("ocaml src/main.ml"),
        Haskell => format!("ghc -o {name} src/main.hs && ./{name}"),
    }
}

pub fn create(dir: &str, name: &str, langs: &[Lang], init_git: bool, github_url: &str) {
    let _ = std::fs::create_dir_all(dir);
    let src = format!("{}/src", dir);
    let _ = std::fs::create_dir_all(&src);

    let mut build_lines: Vec<String> = Vec::new();
    for l in langs {
        let file = format!("{}/main.{}", src, l.ext());
        w(&file, source(*l));
        build_lines.push(howto(*l, name));
    }

    build_lines.push(String::new());
    build_lines.push("Or in AmnesiaIDE: open a source file and press F5.".into());

    let howto_content = format!("=== HOW TO BUILD ===\nProject: {}\n\n{}\n", name, build_lines.join("\n"));
    w(&format!("{}/HOWTO_BUILD.txt", dir), &howto_content);

    if init_git {
        let _ = std::process::Command::new("git").args(["init", dir]).output();
        let gitignore = String::from("target/\n*.o\n*.exe\n*.out\nnode_modules/\n.zig-cache/\n");
        w(&format!("{}/.gitignore", dir), &gitignore);
        if !github_url.is_empty() {
            let _ = std::process::Command::new("git")
                .args(["-C", dir, "remote", "add", "origin", github_url]).output();
            let _ = std::process::Command::new("git")
                .args(["-C", dir, "add", "."]).output();
            let _ = std::process::Command::new("git")
                .args(["-C", dir, "commit", "-m", "Initial commit"]).output();
        }
    }
}
