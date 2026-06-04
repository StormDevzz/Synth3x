use eframe::egui::{self, Color32, Frame, Visuals};
use crate::syntax;

#[derive(Default, PartialEq)]
enum Dialog {
    #[default]
    None,
    Open,
    SaveAs,
}

pub struct AmnesiaApp {
    text: String,
    path: String,
    dirty: bool,
    lang: syntax::Lang,
    show_output: bool,
    output_text: String,
    cursor_line: usize,
    cursor_col: usize,
    dialog: Dialog,
    dialog_path: String,
    dialog_msg: String,
    line_count: usize,
}

impl Default for AmnesiaApp {
    fn default() -> Self {
        Self {
            text: String::new(),
            path: String::new(),
            dirty: false,
            lang: syntax::Lang::None,
            show_output: false,
            output_text: String::new(),
            cursor_line: 0,
            cursor_col: 0,
            dialog: Dialog::None,
            dialog_path: String::new(),
            dialog_msg: String::new(),
            line_count: 1,
        }
    }
}

impl AmnesiaApp {
    fn update_line_count(&mut self) {
        self.line_count = self.text.lines().count().max(1);
    }

    fn open_file(&mut self, path: &str) {
        match std::fs::read_to_string(path) {
            Ok(content) => {
                self.path = path.to_owned();
                self.text = content;
                self.dirty = false;
                self.lang = syntax::detect_lang(path);
                self.dialog_msg = format!("Opened {}", path);
                self.output_text.clear();
                self.update_line_count();
            }
            Err(e) => self.dialog_msg = format!("Error: {}", e),
        }
    }

    fn save_file(&mut self, path: &str) -> bool {
        match std::fs::write(path, &self.text) {
            Ok(_) => {
                self.path = path.to_owned();
                self.dirty = false;
                self.lang = syntax::detect_lang(path);
                self.dialog_msg = "Saved".into();
                true
            }
            Err(e) => { self.dialog_msg = format!("Error: {}", e); false }
        }
    }

    fn compile_run(&mut self) {
        if self.path.is_empty() {
            self.dialog_msg = "Save first (File > Save)".into();
            return;
        }
        let path = self.path.clone();
        self.save_file(&path);
        let cmd = match self.lang {
            syntax::Lang::C      => format!("gcc -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1", path),
            syntax::Lang::Cpp    => format!("g++ -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1", path),
            syntax::Lang::Csharp => format!("mcs '{}' 2>&1 || echo 'C# not found'", path),
            syntax::Lang::Rust   => format!("rustc -o /tmp/amnesia_out '{}' 2>&1", path),
            syntax::Lang::Asm    => format!("nasm -f elf64 '{}' -o /tmp/amnesia_out.o && ld -o /tmp/amnesia_out /tmp/amnesia_out.o 2>&1", path),
            _ => format!("gcc -Wall -Wextra -o /tmp/amnesia_out '{}' 2>&1 || echo 'no compiler'", path),
        };
        self.output_text = "Building...\n".to_string();
        self.show_output = true;
        let output = std::process::Command::new("sh").args(["-c", &cmd]).output();
        match output {
            Ok(out) => {
                let text = format!("{}{}", String::from_utf8_lossy(&out.stdout), String::from_utf8_lossy(&out.stderr));
                if out.status.success() {
                    self.output_text = format!("{}\n── Build OK ──", text);
                } else {
                    self.output_text = format!("{}\n── Build FAILED ──", text);
                }
            }
            Err(e) => self.output_text = format!("Build error: {}", e),
        }
    }

    fn status_str(&self) -> String {
        let dirty_mark = if self.dirty { " *" } else { "" };
        let fname = if self.path.is_empty() {
            "[no name]".to_owned()
        } else {
            self.path.rsplit('/').next().unwrap_or(&self.path).to_owned()
        };
        let lang_s = match self.lang {
            syntax::Lang::None => "",
            syntax::Lang::C => "C",
            syntax::Lang::Cpp => "C++",
            syntax::Lang::Csharp => "C#",
            syntax::Lang::Rust => "Rust",
            syntax::Lang::Asm => "ASM",
        };
        format!("{} {} {}  Ln {}, Col {}", fname, dirty_mark, lang_s, self.cursor_line + 1, self.cursor_col + 1)
    }
}

impl eframe::App for AmnesiaApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        ctx.set_visuals(Visuals::dark());

        // Menu bar
        egui::TopBottomPanel::top("menu").show(ctx, |ui| {
            egui::menu::bar(ui, |ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("Open  Ctrl+O").clicked() {
                        self.dialog = Dialog::Open;
                        self.dialog_path = self.path.clone();
                        ui.close_menu();
                    }
                    if ui.button("Save  Ctrl+S").clicked() {
                        if self.path.is_empty() {
                            self.dialog = Dialog::SaveAs;
                            self.dialog_path.clear();
                        } else {
                            let p = self.path.clone();
                            self.save_file(&p);
                        }
                        ui.close_menu();
                    }
                    if ui.button("Save As...").clicked() {
                        self.dialog = Dialog::SaveAs;
                        self.dialog_path.clear();
                        ui.close_menu();
                    }
                    ui.separator();
                    if ui.button("Quit").clicked() { std::process::exit(0); }
                });
                ui.menu_button("Run", |ui| {
                    if ui.button("Compile & Run  F5").clicked() {
                        self.compile_run();
                        ui.close_menu();
                    }
                });
                if ui.button("Terminal").clicked() {
                    self.show_output = !self.show_output;
                }
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    if !self.dialog_msg.is_empty() {
                        ui.label(&self.dialog_msg);
                    }
                });
            });
        });

        // Status bar
        let status = self.status_str();
        egui::TopBottomPanel::bottom("status").show(ctx, |ui| {
            ui.add(egui::Label::new(
                egui::RichText::new(&status).color(Color32::from_gray(160))
            ));
        });

        // Output panel
        if self.show_output {
            egui::TopBottomPanel::bottom("output")
                .min_height(60.0)
                .resizable(true)
                .show(ctx, |ui| {
                    egui::ScrollArea::vertical()
                        .stick_to_bottom(true)
                        .show(ui, |ui| {
                            ui.set_min_height(120.0);
                            Frame::none()
                                .fill(Color32::from_rgb(20, 18, 30))
                                .show(ui, |ui| {
                                    ui.label(
                                        egui::RichText::new(&self.output_text)
                                            .font(egui::FontId::new(13.0, egui::FontFamily::Monospace))
                                            .color(Color32::from_gray(200))
                                    );
                                });
                        });
                });
        }

        // Dialog
        if self.dialog != Dialog::None {
            let (title, action_label) = match self.dialog {
                Dialog::Open => ("Open File", "Open"),
                Dialog::SaveAs => ("Save File As", "Save"),
                _ => unreachable!(),
            };
            egui::Window::new(title)
                .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
                .collapsible(false)
                .resizable(false)
                .show(ctx, |ui| {
                    ui.horizontal(|ui| {
                        ui.label("Path:");
                        ui.text_edit_singleline(&mut self.dialog_path);
                    });
                    ui.horizontal(|ui| {
                        if ui.button(action_label).clicked() {
                            let p = self.dialog_path.trim().to_owned();
                            if !p.is_empty() {
                                match self.dialog {
                                    Dialog::Open => self.open_file(&p),
                                    Dialog::SaveAs => { self.save_file(&p); },
                                    _ => {}
                                }
                                self.dialog = Dialog::None;
                            }
                        }
                        if ui.button("Cancel").clicked() {
                            self.dialog = Dialog::None;
                        }
                    });
                });
        }

        // Editor
        egui::CentralPanel::default().show(ctx, |ui| {
            let available = ui.available_size();
            let line_digit_w = 36.0;
            let editor_w = (available.x - line_digit_w).max(100.0);

            Frame::none()
                .fill(Color32::from_rgb(30, 28, 40))
                .show(ui, |ui| {
                    ui.set_min_size(available);

                    egui::ScrollArea::vertical()
                        .id_source("editor_scroll")
                        .show(ui, |ui| {
                            ui.horizontal(|ui| {
                                // Line numbers
                                ui.add_sized([line_digit_w, available.y], |ui: &mut egui::Ui| {
                                    ui.set_min_width(line_digit_w);
                                    let total = self.line_count.max(1);
                                    let line_h = 18.0;
                                    for i in 0..total {
                                        let y = i as f32 * line_h;
                                        let rect = egui::Rect::from_min_size(
                                            egui::pos2(4.0, y),
                                            egui::vec2(line_digit_w - 4.0, line_h),
                                        );
                                        let color = if i == self.cursor_line {
                                            Color32::from_rgb(220, 220, 220)
                                        } else {
                                            Color32::from_gray(110)
                                        };
                                        ui.put(rect, egui::Label::new(
                                            egui::RichText::new(format!("{:>3}", i + 1))
                                                .font(egui::FontId::new(13.0, egui::FontFamily::Monospace))
                                                .color(color)
                                        ));
                                    }
                                    ui.allocate_rect(ui.available_rect_before_wrap(), egui::Sense::hover())
                                });

                                // Text editor
                                ui.add_sized([editor_w, available.y], |ui: &mut egui::Ui| {
                                    ui.set_min_width(editor_w);
                                    ui.set_min_height(available.y);

                                    let lang = self.lang;
                                    let mut layouter = move |ui: &egui::Ui, text: &str, _max_width: f32| {
                                        let job = syntax::highlight_job(text, lang);
                                        ui.fonts(|f| f.layout_job(job))
                                    };

                                    let resp = egui::TextEdit::multiline(&mut self.text)
                                        .font(egui::FontId::new(14.0, egui::FontFamily::Monospace))
                                        .desired_width(editor_w)
                                        .layouter(&mut layouter)
                                        .show(ui);

                                    if resp.response.changed() {
                                        self.dirty = true;
                                        self.update_line_count();
                                    }
                                    if let Some(cursor) = resp.cursor_range {
                                        let cc = cursor.primary;
                                        self.cursor_col = cc.rcursor.column;
                                        self.cursor_line = cc.rcursor.row;
                                    }

                                    resp.response
                                });
                            });
                        });
                });
        });

        // Key shortcuts
        ctx.input_mut(|i| {
            if i.consume_key(egui::Modifiers::CTRL, egui::Key::S) {
                if self.path.is_empty() {
                    self.dialog = Dialog::SaveAs;
                    self.dialog_path.clear();
                } else {
                    let p = self.path.clone();
                    self.save_file(&p);
                }
            }
            if i.consume_key(egui::Modifiers::CTRL, egui::Key::O) {
                self.dialog = Dialog::Open;
                self.dialog_path = self.path.clone();
            }
            if i.consume_key(egui::Modifiers::NONE, egui::Key::F5) {
                self.compile_run();
            }
        });

        ctx.request_repaint();
    }
}
