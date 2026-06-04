use eframe::egui::{self, Color32};
use crate::editor::{AmnesiaApp, Dialog, Screen};
use crate::{terminal, syntax};

pub fn welcome(app: &mut AmnesiaApp, ctx: &egui::Context) {
    egui::CentralPanel::default().show(ctx, |ui| {
        ui.vertical_centered_justified(|ui| {
            ui.add_space(120.0);
            ui.heading(egui::RichText::new("AmnesiaIDE").size(36.0).color(Color32::from_rgb(86, 156, 214)));
            ui.label(egui::RichText::new("C · C++ · C# · Rust · Assembly").size(14.0).color(Color32::from_gray(150)));
            ui.add_space(30.0);
            if ui.button(egui::RichText::new("  Open Workspace  ").size(16.0)).clicked() {
                app.dialog = Dialog::OpenWorkspace;
            }
            ui.add_space(8.0);
            if ui.button(egui::RichText::new("  Create Workspace  ").size(16.0)).clicked() {
                app.dialog = Dialog::NewWorkspace;
            }
            if let Some(ws) = &app.ws {
                ui.add_space(20.0);
                let recent = ws.path.rsplit('/').next().unwrap_or(&ws.path).to_owned();
                ui.label(egui::RichText::new(format!("Last: {}", recent)).color(Color32::from_gray(120)));
                if ui.button("Reopen").clicked() {
                    let p = ws.path.clone(); app.open_workspace(&p);
                }
            }
        });
    });
    dialogs(app, ctx);
}

pub fn workspace(app: &mut AmnesiaApp, ctx: &egui::Context) {
    if let Some(ws) = &mut app.ws { ws.term.poll(); }

    // Menu
    egui::TopBottomPanel::top("menu").show(ctx, |ui| {
        egui::menu::bar(ui, |ui| {
            ui.menu_button("File", |ui| {
                if ui.button("New File").clicked() { app.dialog = Dialog::NewFile; ui.close_menu(); }
                if ui.button("Open  Ctrl+O").clicked() { app.dialog = Dialog::Open; app.dialog_path = app.file_path.clone(); ui.close_menu(); }
                if ui.button("Save  Ctrl+S").clicked() {
                    if app.file_path.is_empty() { app.dialog = Dialog::SaveAs; }
                    else { let p = app.file_path.clone(); app.save_file(&p); }
                    ui.close_menu();
                }
                if ui.button("Save As...").clicked() { app.dialog = Dialog::SaveAs; ui.close_menu(); }
                ui.separator();
                if ui.button("Quit").clicked() { std::process::exit(0); }
            });
            ui.menu_button("Workspace", |ui| {
                if ui.button("Open Workspace").clicked() { app.dialog = Dialog::OpenWorkspace; ui.close_menu(); }
                if ui.button("New Workspace").clicked() { app.dialog = Dialog::NewWorkspace; ui.close_menu(); }
                if ui.button("Refresh Files").clicked() { if let Some(ws) = &mut app.ws { ws.refresh(); } ui.close_menu(); }
                ui.separator();
                if ui.button("Close Workspace").clicked() { app.screen = Screen::Welcome; app.msg.clear(); ui.close_menu(); }
            });
            ui.menu_button("Run", |ui| {
                if ui.button("Compile & Run  F5").clicked() { app.compile_run(); ui.close_menu(); }
            });
            ui.toggle_value(&mut app.show_term, "Terminal");
            ui.toggle_value(&mut app.show_files, "Files");
            ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                if !app.msg.is_empty() { ui.label(&app.msg); }
            });
        });
    });

    // Status
    let status = app.status_str();
    egui::TopBottomPanel::bottom("status").show(ctx, |ui| {
        ui.add(egui::Label::new(egui::RichText::new(&status).color(Color32::from_gray(160))));
    });

    // Terminal
    if app.show_term {
        egui::TopBottomPanel::bottom("term").min_height(80.0).resizable(true).show(ctx, |ui| {
            if let Some(ws) = &mut app.ws { terminal::render::show(ui, &mut ws.term); }
        });
    }

    // File tree
    egui::SidePanel::left("files").resizable(true).default_width(160.0).min_width(80.0)
        .show_animated(ctx, app.show_files, |ui| {
            ui.vertical(|ui| {
                ui.label(egui::RichText::new("Workspace").color(Color32::from_rgb(86, 156, 214)).size(12.0));
                if ui.button("+ New File").clicked() { app.dialog = Dialog::NewFile; }
                ui.separator();
                egui::ScrollArea::vertical().show(ui, |ui| {
                    let entries: Vec<_> = app.ws.as_ref().map(|ws| ws.files.clone()).unwrap_or_default();
                    for f in &entries {
                        let indent = "  ".repeat(f.depth as usize);
                        let icon = if f.is_dir { "📁" } else { file_icon(&f.name) };
                        let f_path = f.path.clone();
                        let label = format!("{}{} {}", indent, icon, f.name);
                        let resp = ui.add_sized([ui.available_width(), 20.0],
                            egui::Label::new(egui::RichText::new(&label).size(13.0))
                                .sense(egui::Sense::click()));
                        if resp.clicked() && !f.is_dir { app.open_file(&f_path); }
                    }
                });
            });
        });

    // Dialogs
    dialogs(app, ctx);

    // Editor
    egui::CentralPanel::default().show(ctx, |ui| {
        let lang = app.lang;
        let mut layouter = move |ui: &egui::Ui, text: &str, _: f32| {
            let job = syntax::highlight_job(text, lang);
            ui.fonts(|f| f.layout_job(job))
        };
        let resp = egui::TextEdit::multiline(&mut app.text)
            .font(egui::FontId::new(14.0, egui::FontFamily::Monospace))
            .layouter(&mut layouter)
            .show(ui);
        if resp.response.changed() { app.dirty = true; }
        if let Some(cu) = resp.cursor_range {
            app.cursor_col = cu.primary.rcursor.column;
            app.cursor_line = cu.primary.rcursor.row;
        }
    });

    // Hotkeys
    ctx.input_mut(|i| {
        if i.consume_key(egui::Modifiers::CTRL, egui::Key::S) {
            if app.file_path.is_empty() { app.dialog = Dialog::SaveAs; }
            else { let p = app.file_path.clone(); app.save_file(&p); }
        }
        if i.consume_key(egui::Modifiers::CTRL, egui::Key::O) {
            app.dialog = Dialog::Open; app.dialog_path = app.file_path.clone();
        }
        if i.consume_key(egui::Modifiers::NONE, egui::Key::F5) { app.compile_run(); }
    });
}

fn file_icon(name: &str) -> &'static str {
    if let Some(ext) = name.rsplit('.').next() {
        match ext {
            "rs" => "🦀",
            "c" | "h" => "⚡",
            "cpp" | "cc" | "cxx" | "hpp" | "hxx" => "⚡",
            "cs" => "🔷",
            "asm" | "s" | "S" => "⚙",
            "py" => "🐍",
            "js" | "ts" => "⬡",
            "html" | "htm" => "🌐",
            "css" | "scss" => "🎨",
            "md" | "txt" => "📄",
            "toml" | "json" | "yaml" | "yml" => "📋",
            "png" | "jpg" | "jpeg" | "gif" | "svg" => "🖼",
            "lock" | "gitignore" => "🔒",
            _ => "📄",
        }
    } else {
        "📄"
    }
}

fn dialogs(app: &mut AmnesiaApp, ctx: &egui::Context) {
    let d = match app.dialog {
        Dialog::None => return,
        Dialog::Open => ("Open File", "Open"),
        Dialog::SaveAs => ("Save File As", "Save"),
        Dialog::NewWorkspace => ("Create Workspace", "Create"),
        Dialog::OpenWorkspace => ("Open Workspace", "Open"),
        Dialog::NewFile => ("New File", "Create"),
    };
    let (title, action) = d;
    egui::Window::new(title)
        .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
        .collapsible(false)
        .resizable(false)
        .show(ctx, |ui| {
            ui.horizontal(|ui| { ui.label("Path:"); ui.text_edit_singleline(&mut app.dialog_path); });
            ui.horizontal(|ui| {
                if ui.button(action).clicked() {
                    let p = app.dialog_path.trim().to_owned();
                    if !p.is_empty() {
                        match app.dialog {
                            Dialog::Open => app.open_file(&p),
                            Dialog::SaveAs => { app.save_file(&p); },
                            Dialog::NewWorkspace => {
                                let _ = std::fs::create_dir_all(&p);
                                app.open_workspace(&p);
                            }
                            Dialog::OpenWorkspace => { app.open_workspace(&p); },
                            Dialog::NewFile => { app.new_file(&p); },
                            _ => {}
                        }
                        app.dialog = Dialog::None;
                    }
                }
                if ui.button("Cancel").clicked() { app.dialog = Dialog::None; }
            });
        });
}
