use eframe::egui::{self, Color32};
use crate::editor::{AmnesiaApp, Dialog, Screen};
use crate::{terminal, syntax, notifications};
use crate::creations::actions;

pub fn welcome(app: &mut AmnesiaApp, ctx: &egui::Context) {
    egui::CentralPanel::default().show(ctx, |ui| {
        ui.vertical_centered_justified(|ui| {
            ui.add_space(120.0);
            ui.heading(egui::RichText::new("AmnesiaIDE").size(36.0).color(Color32::from_rgb(86, 156, 214)));
            ui.label(egui::RichText::new("C · C++ · C# · Rust · Assembly").size(14.0).color(Color32::from_gray(150)));
            ui.add_space(30.0);
            if ui.button(egui::RichText::new("  New Project  ").size(16.0)).clicked() {
                app.new_project.visible = true;
            }
            ui.add_space(8.0);
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
    crate::dialog::show(app, ctx);
    dialogs(app, ctx);
    notifications::toast::show(ctx, &mut app.notify);

    // Escape on welcome screen
    ctx.input_mut(|i| {
        if i.consume_key(egui::Modifiers::NONE, egui::Key::Escape) {
            app.dialog = Dialog::None;
            app.new_project.visible = false;
        }
    });
}

pub fn workspace(app: &mut AmnesiaApp, ctx: &egui::Context) {
    if let Some(ws) = &mut app.ws { ws.term.poll(); }

    // Menu
    egui::TopBottomPanel::top("menu").show(ctx, |ui| {
        egui::menu::bar(ui, |ui| {
            ui.menu_button("File", |ui| {
                if ui.button("New File").clicked() { app.dialog = Dialog::NewFile; ui.close_menu(); }
                if ui.button("New Project...").clicked() { app.new_project.visible = true; ui.close_menu(); }
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
                if ui.button("Refresh Files").clicked() {
                    if let Some(ws) = &mut app.ws { ws.refresh(); } ui.close_menu();
                }
                if ui.button("Re-check Compilers").clicked() {
                    app.notify.push(crate::notifications::store::Level::Info, "Checking compilers...".into());
                    crate::install::check::check_all(&mut app.notify);
                    ui.close_menu();
                }
                ui.separator();
                if ui.button("Close Workspace").clicked() { app.screen = Screen::Welcome; app.msg.clear(); ui.close_menu(); }
            });
            ui.menu_button("GitHub", |ui| {
                if ui.button("Open GitHub Panel").clicked() { app.github.visible = true; ui.close_menu(); }
                if ui.button("Status").clicked() {
                    if let Some(ws) = &app.ws {
                        app.github.output = crate::github::gh::run(&["status", &ws.path]);
                    }
                    ui.close_menu();
                }
                if ui.button("Pull").clicked() {
                    if let Some(ws) = &app.ws {
                        app.github.output = crate::github::gh::run(&["pull", &ws.path]);
                    }
                    ui.close_menu();
                }
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
    egui::SidePanel::left("files").resizable(true).default_width(180.0).min_width(80.0)
        .show_animated(ctx, app.show_files, |ui| {
            ui.vertical(|ui| {
                ui.horizontal(|ui| {
                    ui.label(egui::RichText::new("Workspace").color(Color32::from_rgb(86, 156, 214)).size(12.0));
                    if ui.button("+").clicked() { app.dialog = Dialog::NewFile; }
                });
                ui.separator();
                egui::ScrollArea::vertical().show(ui, |ui| {
                    let entries: Vec<_> = app.ws.as_ref().map(|ws| ws.files.clone()).unwrap_or_default();
                    for f in &entries {
                        ui.horizontal(|ui| {
                            ui.add(egui::Label::new("  ".repeat(f.depth as usize)));

                            let icon_tex = if f.is_dir { app.icons.dir.as_ref() } else { app.icons.for_file(&f.name) };
                            if let Some(tex) = icon_tex {
                                let size = egui::Vec2::splat(16.0);
                                ui.add(egui::Image::new((tex.id(), size)));
                            } else {
                                ui.label(crate::icon::file::for_entry(&f));
                            }

                            let resp = ui.add_sized(
                                [ui.available_width(), 18.0],
                                egui::Button::new(egui::RichText::new(&f.name).size(13.0)).frame(false),
                            );

                            if resp.clicked() && !f.is_dir {
                                app.open_file(&f.path);
                            }
                            if resp.secondary_clicked() {
                                app.ctx_file = Some(f.clone());
                                ui.close_menu();
                            }
                        });
                    }
                });
            });
        });

    // Context menu for file tree items
    if let Some(f) = &app.ctx_file.clone() {
        let f_path = f.path.clone();
        let f_name = f.name.clone();
        egui::Area::new("ctx_menu".into())
            .interactable(true)
            .show(ctx, |ui| {
                egui::Frame::popup(ui.style()).show(ui, |ui| {
                    if ui.button("Open").clicked() { app.open_file(&f_path); app.ctx_file = None; }
                    if ui.button("Delete").clicked() {
                        match actions::delete_file(&f_path) {
                            Ok(_) => {
                                app.notify.push(crate::notifications::store::Level::Info, format!("Deleted {}", f_name));
                                if let Some(ws) = &mut app.ws { ws.refresh(); }
                                if app.file_path == f_path { app.file_path.clear(); app.text.clear(); }
                            }
                            Err(e) => app.notify.push(crate::notifications::store::Level::Error, e),
                        }
                        app.ctx_file = None;
                    }
                    if ui.button("Rename").clicked() {
                        app.rename_target = Some(f_path.clone());
                        app.rename_buf = f_name.clone();
                        app.ctx_file = None;
                    }
                    if ui.button("Close").clicked() { app.ctx_file = None; }
                });
            });
    }

    // Rename dialog
    if let Some(ref target) = app.rename_target.clone() {
        egui::Window::new("Rename")
            .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
            .collapsible(false)
            .resizable(false)
            .show(ctx, |ui| {
                ui.horizontal(|ui| { ui.label("New name:"); ui.text_edit_singleline(&mut app.rename_buf); });
                ui.horizontal(|ui| {
                    if ui.button("Rename").clicked() {
                        let new_name = app.rename_buf.trim().to_owned();
                        if !new_name.is_empty() {
                            let parent = std::path::Path::new(&target).parent()
                                .map(|p| p.to_string_lossy().to_string()).unwrap_or_default();
                            let new_path = if parent.is_empty() { new_name } else { format!("{}/{}", parent, new_name) };
                            match actions::rename_file(&target, &new_path) {
                                Ok(_) => {
                                    app.notify.push(crate::notifications::store::Level::Info, "Renamed".into());
                                    if let Some(ws) = &mut app.ws { ws.refresh(); }
                                    if app.file_path == *target { app.open_file(&new_path); }
                                }
                                Err(e) => app.notify.push(crate::notifications::store::Level::Error, e),
                            }
                        }
                        app.rename_target = None;
                    }
                    if ui.button("Cancel").clicked() { app.rename_target = None; }
                });
            });
    }

    // GitHub panel
    crate::github::show(app, ctx);

    // Dialogs
    crate::dialog::show(app, ctx);
    dialogs(app, ctx);

    // Toast notifications
    notifications::toast::show(ctx, &mut app.notify);

    // File header
    if !app.file_path.is_empty() {
        egui::TopBottomPanel::top("file_header").min_height(24.0).show(ctx, |ui| {
            let fname = app.file_path.rsplit('/').next().unwrap_or(&app.file_path);
            let dm = if app.dirty { " ●" } else { "" };
            ui.horizontal(|ui| {
                ui.add(egui::Label::new(
                    egui::RichText::new(format!("{}{}", fname, dm))
                        .color(Color32::from_rgb(200, 200, 200))
                        .size(13.0)
                ));
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    ui.label(egui::RichText::new(&app.file_path).color(Color32::from_gray(120)).size(11.0));
                });
            });
        });
    }

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
        if i.consume_key(egui::Modifiers::NONE, egui::Key::Escape) {
            app.dialog = Dialog::None;
            app.ctx_file = None;
            app.rename_target = None;
            app.new_project.visible = false;
            app.github.visible = false;
        }
    });
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
                            Dialog::NewWorkspace => { let _ = std::fs::create_dir_all(&p); app.open_workspace(&p); }
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
