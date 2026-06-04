use crate::ccore;

const TAB: &str = "    ";

pub struct Editor {
    lines: Vec<String>,
    cx: i32, cy: i32,
    rows: i32, cols: i32,
    rowoff: i32, coloff: i32,
    path: String,
    dirty: bool,
    msg: String,
    lang: i32,
    term_mode: bool,
    term_lines: Vec<String>,
    term_scroll: i32,
    term_buf: String,
}

impl Editor {
    pub fn new(content: String, path: String) -> Self {
        let lines: Vec<String> = if content.is_empty() {
            vec![String::new()]
        } else {
            content.lines().map(|l| l.to_string()).collect()
        };
        let (rows, cols) = ccore::get_size();
        let lang = ccore::detect_lang(&path);
        Editor {
            lines, cx: 0, cy: 0, rows, cols,
            rowoff: 0, coloff: 0,
            path, dirty: false,
            msg: String::new(), lang,
            term_mode: false,
            term_lines: vec![String::new()],
            term_scroll: 0, term_buf: String::new(),
        }
    }

    pub fn run(&mut self) {
        loop {
            self.refresh();
            let key = ccore::read_key();
            if key < 0 { continue; }
            if !self.handle_key(key) { break; }
        }
    }

    fn editor_rows(&self) -> i32 {
        if self.term_mode { self.rows / 3 } else { self.rows - 2 }
    }

    fn term_rows(&self) -> i32 {
        if self.term_mode { self.rows - self.editor_rows() - 2 } else { 0 }
    }

    fn refresh(&self) {
        let mut buf = String::new();
        buf.push_str("\x1b[H");
        let erows = self.editor_rows();

        for i in 0..erows {
            let fi = self.rowoff + i;
            if fi >= 0 && (fi as usize) < self.lines.len() {
                buf.push_str(&self.render_line(fi as usize, self.cols as usize));
            }
            buf.push_str("\x1b[K");
            if i < erows - 1 { buf.push_str("\r\n"); }
        }

        buf.push_str(&format!("\x1b[{};1H\x1b[47;30m", erows + 1));
        let fname = if self.path.is_empty() { "[no name]" } else {
            self.path.rsplit('/').next().unwrap_or(&self.path)
        };
        let lang_s = ["", "C", "C++", "C#", "Rust", "ASM"][self.lang as usize];
        let tab = format!("  {} {}  {}:{}  {}  ",
            fname, if self.dirty { "\x1b[91m*\x1b[30m" } else { " " },
            self.cy + 1, self.cx + 1, lang_s);
        buf.push_str(&tab);
        buf.push_str("\x1b[K");

        if self.term_mode {
            let tstart = erows + 2;
            let trows = self.term_rows();
            buf.push_str(&format!("\x1b[{};1H\x1b[44;37m  TERMINAL  \x1b[K\x1b[0m", tstart - 1));
            for i in 0..trows {
                let ti = self.term_scroll + i;
                buf.push_str(&format!("\x1b[{};1H", tstart + i));
                if ti >= 0 && (ti as usize) < self.term_lines.len() {
                    buf.push_str(&format!("\x1b[90m{:>3}\x1b[0m {}", ti, self.term_lines[ti as usize]));
                }
                buf.push_str("\x1b[K");
            }
            let prompt_row = tstart + trows;
            buf.push_str(&format!("\x1b[{};1H\x1b[42;30m  $ {} \x1b[K\x1b[0m", prompt_row, self.term_buf));
        } else {
            buf.push_str(&format!("\x1b[{};1H\x1b[44;37m", self.rows));
            if !self.msg.is_empty() {
                buf.push_str(&format!("  {}", self.msg));
            } else {
                buf.push_str("  ^O open  ^S save  ^R run  F4 term  ^Q quit  ^G goto  ^K kill");
            }
            buf.push_str("\x1b[K\x1b[0m");
            buf.push_str(&format!("\x1b[{};{}H", self.cy - self.rowoff + 1, (self.cx - self.coloff + 1).max(1)));
        }
        ccore::write_str(&buf);
    }

    fn render_line(&self, line: usize, max_col: usize) -> String {
        let mut out = String::new();
        let s = if line < self.lines.len() { &self.lines[line] } else { "" };
        out.push_str(&format!("\x1b[90m{:>4}\x1b[0m ", line + 1));
        let chars: Vec<char> = s.chars().collect();
        let mut i = self.coloff as usize;
        let start = out.len();
        while i < chars.len() && (out.len() - start) < max_col.saturating_sub(5) {
            if let Some(color) = ccore::get_syntax_color(s, i as i32) {
                out.push_str(color);
            }
            match chars[i] {
                '\t' => out.push_str(TAB),
                c if (c as u32) < 32 => {
                    out.push_str(&format!("\x1b[7m^{}\x1b[0m", (c as u8 + 64) as char));
                }
                c => out.push(c),
            }
            if ccore::get_syntax_color(s, i as i32).is_some() {
                out.push_str("\x1b[0m");
            }
            i += 1;
        }
        out
    }

    fn handle_key(&mut self, key: i32) -> bool {
        self.msg.clear();
        if self.term_mode { return self.handle_term_key(key); }

        match key {
            27 => {
                let k2 = ccore::read_key();
                if k2 == 79 {
                    let k3 = ccore::read_key();
                    match k3 {
                        80 | 81 | 82 => { self.term_mode = true; self.term_buf.clear(); return true; }
                        83 => { self.term_mode = !self.term_mode; self.term_buf.clear(); return true; }
                        _ => {}
                    }
                } else if k2 == 91 {
                    let k3 = ccore::read_key();
                    match k3 {
                        65 => self.move_cursor(-1, 0),
                        66 => self.move_cursor(1, 0),
                        67 => self.move_cursor(0, 1),
                        68 => self.move_cursor(0, -1),
                        72 => { self.cx = 0; self.coloff = 0; }
                        70 => { self.cx = self.lines[self.cy as usize].len() as i32; self.scroll_horiz(); }
                        _ => {}
                    }
                }
            }
            15 => self.open_file(),
            19 => self.save_file(),
            18 => self.compile_run(),
            17 => return false,
            7 => self.prompt_goto(),
            11 => self.kill_line(),
            21 => { self.lines[self.cy as usize].clear(); self.cx = 0; self.dirty = true; }
            10 => self.insert_newline(),
            127 => self.delete_char(),
            9 => self.insert_str(TAB),
            c if c >= 32 => { self.insert_char(c as u8 as char); }
            _ => {}
        }
        true
    }

    fn handle_term_key(&mut self, key: i32) -> bool {
        match key {
            27 => {
                let k2 = ccore::read_key();
                if k2 == 79 && ccore::read_key() == 83 { self.term_mode = false; return true; }
                if k2 == 91 { ccore::read_key(); return true; }
                self.term_mode = false;
                return true;
            }
            10 => {
                let cmd = std::mem::take(&mut self.term_buf);
                self.term_lines.push(format!("$ {}", cmd));
                if !cmd.is_empty() {
                    let output = std::process::Command::new("sh")
                        .args(["-c", &cmd]).output().ok();
                    if let Some(out) = output {
                        for l in String::from_utf8_lossy(&out.stdout).lines() {
                            self.term_lines.push(l.to_string());
                        }
                        for l in String::from_utf8_lossy(&out.stderr).lines() {
                            self.term_lines.push(format!("\x1b[91m{}\x1b[0m", l));
                        }
                    } else {
                        self.term_lines.push("\x1b[91mfailed\x1b[0m".to_string());
                    }
                }
                self.term_lines.push(String::new());
                let maxl = self.term_rows().max(4) as usize * 4;
                if self.term_lines.len() > maxl {
                    self.term_lines.drain(0..self.term_lines.len() - maxl);
                }
                self.term_scroll = (self.term_lines.len() as i32 - self.term_rows()).max(0);
            }
            127 => { self.term_buf.pop(); }
            c if c >= 32 => { self.term_buf.push(c as u8 as char); }
            _ => {}
        }
        true
    }

    fn open_file(&mut self) {
        let p = self.prompt("Open:");
        if p.is_empty() { return; }
        match std::fs::read_to_string(&p) {
            Ok(content) => {
                self.lines = if content.is_empty() {
                    vec![String::new()]
                } else { content.lines().map(|l| l.to_string()).collect() };
                self.path = p; self.cx = 0; self.cy = 0;
                self.rowoff = 0; self.coloff = 0; self.dirty = false;
                self.lang = ccore::detect_lang(&self.path);
                self.msg = format!("Opened {} ({} lines)", self.path, self.lines.len());
            }
            Err(e) => self.msg = format!("Error: {}", e),
        }
    }

    fn save_file(&mut self) {
        let path = if self.path.is_empty() {
            let p = self.prompt("Save as:");
            if p.is_empty() { return; } p
        } else { self.path.clone() };
        match std::fs::write(&path, self.lines.join("\n")) {
            Ok(_) => { self.path = path; self.dirty = false; self.lang = ccore::detect_lang(&self.path); self.msg = "Saved".into(); }
            Err(e) => self.msg = format!("Error: {}", e),
        }
    }

    fn compile_run(&mut self) {
        if self.path.is_empty() { self.msg = "Save first (^S)".into(); return; }
        self.save_file();
        let cmd = match self.lang {
            1 => format!("gcc -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1", self.path),
            2 => format!("g++ -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1", self.path),
            3 => format!("mcs '{}' 2>&1 || echo 'C# not found'", self.path),
            4 => format!("rustc -o /tmp/amnesia_out '{}' 2>&1", self.path),
            5 => format!("nasm -f elf64 '{}' -o /tmp/amnesia_out.o && ld -o /tmp/amnesia_out /tmp/amnesia_out.o 2>&1", self.path),
            _ => format!("gcc -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1 || echo 'no compiler'", self.path),
        };
        self.msg = "Building...".into();
        self.refresh();
        let output = std::process::Command::new("sh").args(["-c", &cmd]).output().ok();
        if let Some(out) = output {
            let text = format!("{}{}", String::from_utf8_lossy(&out.stdout), String::from_utf8_lossy(&out.stderr));
            let ok = out.status.success();
            self.msg = if ok { "Build OK".into() } else { "Build FAILED".into() };
            self.term_mode = true;
            self.term_lines.clear();
            self.term_buf.clear();
            for l in text.lines() {
                self.term_lines.push(if ok { l.to_string() } else { format!("\x1b[91m{}\x1b[0m", l) });
            }
            self.term_lines.push(String::new());
            self.term_lines.push(if ok { "\x1b[92m== OK ==\x1b[0m".into() } else { "\x1b[91m== FAILED ==\x1b[0m".into() });
            self.term_lines.push(String::new());
            self.term_scroll = (self.term_lines.len() as i32 - self.term_rows()).max(0);
        } else { self.msg = "Build error".into(); }
    }

    fn move_cursor(&mut self, dr: i32, dc: i32) {
        let ny = self.cy + dr;
        if ny >= 0 && ny < self.lines.len() as i32 { self.cy = ny; }
        self.cx = (self.cx + dc).max(0).min(self.lines[self.cy as usize].len() as i32);
        self.scroll_vert(); self.scroll_horiz();
    }

    fn scroll_vert(&mut self) {
        let er = self.editor_rows();
        if self.cy < self.rowoff { self.rowoff = self.cy; }
        if self.cy >= self.rowoff + er { self.rowoff = self.cy - er + 1; }
    }

    fn scroll_horiz(&mut self) {
        let w = (self.cols as usize).saturating_sub(6) as i32;
        if self.cx < self.coloff { self.coloff = self.cx; }
        if self.cx >= self.coloff + w { self.coloff = self.cx - w + 1; }
    }

    fn insert_char(&mut self, c: char) { let r = self.cy as usize; self.lines[r].insert(self.cx as usize, c); self.cx += 1; self.dirty = true; }
    fn insert_str(&mut self, s: &str) { for c in s.chars() { let r = self.cy as usize; self.lines[r].insert(self.cx as usize, c); self.cx += 1; } self.dirty = true; }

    fn delete_char(&mut self) {
        if self.cx > 0 { let r = self.cy as usize; self.cx -= 1; self.lines[r].remove(self.cx as usize); self.dirty = true; }
        else if self.cy > 0 {
            let r = self.cy as usize;
            let prev = self.lines[r - 1].len();
            let rest = self.lines.remove(r);
            self.cy -= 1; self.cx = prev as i32;
            self.lines[self.cy as usize].push_str(&rest);
            self.dirty = true;
        }
    }

    fn insert_newline(&mut self) {
        let r = self.cy as usize;
        let rest = self.lines[r].split_off(self.cx as usize);
        self.cy += 1; self.cx = 0;
        self.lines.insert(r + 1, rest);
        self.scroll_vert(); self.dirty = true;
    }

    fn kill_line(&mut self) {
        if self.lines.is_empty() { return; }
        self.lines.remove(self.cy as usize);
        self.dirty = true;
        if self.lines.is_empty() { self.lines.push(String::new()); }
        if self.cy >= self.lines.len() as i32 { self.cy = self.lines.len() as i32 - 1; }
        self.cx = self.cx.min(self.lines[self.cy as usize].len() as i32);
    }

    fn prompt_goto(&mut self) {
        if let Ok(n) = self.prompt("Go to line:").parse::<i32>() {
            self.cy = n.saturating_sub(1).max(0).min(self.lines.len() as i32 - 1);
            self.cx = 0; self.scroll_vert();
        }
    }

    fn prompt(&mut self, prompt: &str) -> String {
        let mut input = String::new();
        loop {
            let buf = format!("\x1b[{};1H\x1b[43;30m  {} {} \x1b[K\x1b[0m\x1b[{};{}H",
                self.rows, prompt, input, self.rows, 5 + prompt.len() + input.len());
            ccore::write_str(&buf);
            let key = ccore::read_key();
            if key < 0 { continue; }
            match key {
                10 => return input,
                27 => return String::new(),
                127 => { input.pop(); }
                c if c >= 32 => input.push(c as u8 as char),
                _ => {}
            }
        }
    }
}
