#[allow(dead_code)]
pub fn template(ext: &str) -> &'static str {
    match ext {
        "c"    => "#include <stdio.h>\n\nint main() {\n    printf(\"hello\\n\");\n    return 0;\n}\n",
        "cpp"  => "#include <iostream>\n\nint main() {\n    std::cout << \"hello\" << std::endl;\n    return 0;\n}\n",
        "cs"   => "using System;\n\nclass Program {\n    static void Main() {\n        Console.WriteLine(\"hello\");\n    }\n}\n",
        "rs"   => "fn main() {\n    println!(\"hello\");\n}\n",
        "asm"  => "section .data\n    msg db 'hello', 0xa\nsection .text\n    global _start\n_start:\n    mov rax, 1\n    mov rdi, 1\n    mov rsi, msg\n    mov rdx, 6\n    syscall\n    mov rax, 60\n    xor rdi, rdi\n    syscall\n",
        "py"   => "def main():\n    print(\"hello\")\n\nif __name__ == \"__main__\":\n    main()\n",
        _      => "",
    }
}
