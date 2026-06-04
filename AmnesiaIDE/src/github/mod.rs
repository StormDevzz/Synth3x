pub mod gh;

use eframe::egui::{self, Color32};
use crate::editor::AmnesiaApp;
use crate::bridge::GoBridge;

pub struct GitHubState {
    pub visible: bool,
    pub clone_url: String,
    pub commit_msg: String,
    pub output: String,
    pub bridge: Option<GoBridge>,
}

impl Default for GitHubState {
    fn default() -> Self {
        Self {
            visible: false,
            clone_url: String::new(),
            commit_msg: String::new(),
            output: String::new(),
            bridge: GoBridge::load(),
        }
    }
}

pub fn show(app: &mut AmnesiaApp, ctx: &egui::Context) {
    if !app.github.visible { return; }
    let ws_path = app.ws.as_ref().map(|w| w.path.clone()).unwrap_or_default();
    let use_bridge = app.github.bridge.is_some();

    egui::Window::new("GitHub")
        .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
        .resizable(false)
        .show(ctx, |ui| {
            ui.label(egui::RichText::new(if use_bridge { "Go Bridge: active" } else { "Go Bridge: not loaded, using CLI fallback" }).size(11.0).color(Color32::from_gray(160)));
            ui.separator();
            ui.heading("Clone Repository");
            ui.horizontal(|ui| {
                ui.label("URL:");
                ui.text_edit_singleline(&mut app.github.clone_url);
                if ui.button("Clone").clicked() && !app.github.clone_url.is_empty() {
                    let url = app.github.clone_url.clone();
                    let home = app.home.clone();
                    app.github.output = if let Some(ref bridge) = app.github.bridge {
                        bridge.clone_repo(&url, &home)
                    } else {
                        gh::run(&["clone", &url, &home])
                    };
                }
            });
            ui.separator();
            if !ws_path.is_empty() {
                ui.heading("Current Workspace");
                if ui.button("Status").clicked() {
                    app.github.output = if let Some(ref bridge) = app.github.bridge {
                        bridge.status(&ws_path)
                    } else {
                        gh::run(&["status", &ws_path])
                    };
                }
                ui.add_space(4.0);
                ui.horizontal(|ui| {
                    ui.label("Commit message:");
                    ui.text_edit_singleline(&mut app.github.commit_msg);
                    if ui.button("Commit & Push").clicked() && !app.github.commit_msg.is_empty() {
                        let msg = app.github.commit_msg.clone();
                        app.github.output = if let Some(ref bridge) = app.github.bridge {
                            bridge.commit_push(&ws_path, &msg)
                        } else {
                            gh::run(&["commit-push", &ws_path, &msg])
                        };
                        app.github.commit_msg.clear();
                    }
                });
                if ui.button("Pull").clicked() {
                    app.github.output = if let Some(ref bridge) = app.github.bridge {
                        bridge.pull(&ws_path)
                    } else {
                        gh::run(&["pull", &ws_path])
                    };
                }
            }
            ui.separator();
            if !app.github.output.is_empty() {
                egui::ScrollArea::vertical().max_height(120.0).show(ui, |ui| {
                    ui.label(egui::RichText::new(&app.github.output).color(Color32::from_gray(200)).size(13.0));
                });
            }
            if ui.button("Close").clicked() { app.github.visible = false; }
        });
}
