use crate::editor::{AmnesiaApp, Screen};
use crate::syntax::{self, Lang};
use crate::file;

impl AmnesiaApp {
    pub fn open_workspace(&mut self, path: &str) {
        match file::Workspace::open(path) {
            Ok(ws) => {
                self.ws = Some(ws);
                self.msg = format!("Workspace: {}", path);
                self.screen = Screen::Workspace;
            }
            Err(e) => self.msg = e,
        }
    }

    pub fn open_file(&mut self, path: &str) {
        match file::ops::read_file(path) {
            Ok(c) => {
                self.file_path = path.to_owned();
                self.text = c;
                self.dirty = false;
                self.lang = syntax::detect_lang(path);
                self.msg = format!("Opened {}", path);
                self.cursor_line = 0; self.cursor_col = 0;
            }
            Err(e) => self.msg = e,
        }
    }

    pub fn save_file(&mut self, path: &str) -> bool {
        match file::ops::write_file(path, &self.text) {
            Ok(_) => {
                let was_new = self.file_path.is_empty();
                self.file_path = path.to_owned();
                self.dirty = false;
                self.lang = syntax::detect_lang(path);
                self.msg = "Saved".into();
                if was_new {
                    if let Some(ws) = &mut self.ws { ws.refresh(); }
                }
                true
            }
            Err(e) => { self.msg = e; false }
        }
    }

    pub fn new_file(&mut self, name: &str) {
        if name.is_empty() { return; }
        let ws_path = self.ws.as_ref().map(|w| w.path.clone()).unwrap_or_default();
        let path = if ws_path.is_empty() { name.to_owned() } else { format!("{}/{}", ws_path, name) };
        match file::ops::create_file(&path) {
            Ok(_) => {
                self.open_file(&path);
                if let Some(ws) = &mut self.ws { ws.refresh(); }
                self.msg = format!("Created {}", name);
            }
            Err(e) => self.msg = e,
        }
    }

    pub fn compile_run(&mut self) {
        if self.file_path.is_empty() { self.msg = "Save first".into(); return; }
        let path = self.file_path.clone();
        self.save_file(&path);
        let cmd = match self.lang {
            Lang::C      => format!("gcc -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1", path),
            Lang::Cpp    => format!("g++ -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1", path),
            Lang::Csharp => format!("mcs '{}' 2>&1 || echo 'C# not found'", path),
            Lang::Rust   => format!("rustc -o /tmp/amnesia_out '{}' 2>&1", path),
            Lang::Asm    => format!("nasm -f elf64 '{}' -o /tmp/amnesia_out.o && ld -o /tmp/amnesia_out /tmp/amnesia_out.o 2>&1", path),
            _ => format!("gcc -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1 || echo 'no compiler'", path),
        };
        self.show_term = true;
        if let Some(ws) = &mut self.ws {
            ws.term.output.push_str(&format!("\n$ {}\n", cmd));
            ws.term.exec(&cmd);
        }
    }

    pub fn status_str(&self) -> String {
        let dm = if self.dirty { " *" } else { "" };
        let fn_ = if self.file_path.is_empty() { "[no name]".into() }
            else { self.file_path.rsplit('/').next().unwrap_or(&self.file_path).to_owned() };
        let ls = match self.lang {
            Lang::None => "",
            Lang::C => "C", Lang::Cpp => "C++",
            Lang::Csharp => "C#", Lang::Rust => "Rust",
            Lang::Asm => "ASM",
        };
        format!("{} {} {}  Ln {}, Col {}", fn_, dm, ls, self.cursor_line + 1, self.cursor_col + 1)
    }
}
