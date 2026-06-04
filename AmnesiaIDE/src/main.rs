use std::fs;

mod editor;
mod ccore;

fn print_logo() {
    let logo = concat!(
        "\x1b[36m    █████╗ ███╗   ███╗███╗   ██╗███████╗███████╗██╗ █████╗ \n",
        "\x1b[36m   ██╔══██╗████╗ ████║████╗  ██║██╔════╝██╔════╝██║██╔══██╗\n",
        "\x1b[35m   ███████║██╔████╔██║██╔██╗ ██║█████╗  ███████╗██║███████║\n",
        "\x1b[35m   ██╔══██║██║╚██╔╝██║██║╚██╗██║██╔══╝  ╚════██║██║██╔══██║\n",
        "\x1b[34m   ██║  ██║██║ ╚═╝ ██║██║ ╚████║███████╗███████║██║██║  ██║\n",
        "\x1b[34m   ╚═╝  ╚═╝╚═╝     ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝╚═╝╚═╝  ╚═╝\x1b[0m",
    );
    println!("{}", logo);
    println!("\x1b[90m      ──  IDE  ·  C  ·  C++  ·  C#  ·  Rust  ·  ASM  ──\x1b[0m\n");
    println!("\x1b[90m      ^O open   ^S save   ^R run    F4 term   ^Q quit\x1b[0m\n");
}

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() > 1 && args[1] == "--help" {
        print_logo();
        println!("Usage: amnesia-ide [file]");
        println!("  Open a file or start a new buffer in the AmnesiaIDE editor.\n");
        println!("Keys:");
        println!("  ^O  Open file        ^S  Save        ^R  Run/compile");
        println!("  ^Q  Quit             ^G  Go to line  ^K  Kill line");
        println!("  F4  Toggle terminal  F1-F3  Terminal\n");
        return;
    }

    let path = if args.len() > 1 && !args[1].starts_with("--") {
        args[1].clone()
    } else {
        String::new()
    };

    let content = if !path.is_empty() {
        fs::read_to_string(&path).unwrap_or_default()
    } else {
        String::new()
    };

    let mut ed = editor::Editor::new(content, path);

    ccore::raw_mode(true);
    ccore::hide_cursor(true);
    ed.run();
    ccore::hide_cursor(false);
    ccore::raw_mode(false);
}
