pub mod gh;

use eframe::egui::{self, Color32};
use crate::editor::AmnesiaApp;
use crate::bridge::GoBridge;

pub struct GitHubState {
    pub visible: bool,
    pub clone_url: String,
    pub commit_msg: String,
    pub token: String,
    pub output: String,
    pub dns_host: String,
    pub http_url: String,
    pub mc_address: String,
    pub mc_player_name_input: String,
    pub mc_uuid_input: String,
    pub bridge: Option<GoBridge>,
}

impl Default for GitHubState {
    fn default() -> Self {
        Self {
            visible: false,
            clone_url: String::new(),
            commit_msg: String::new(),
            token: String::new(),
            output: String::new(),
            dns_host: String::from("github.com"),
            http_url: String::new(),
            mc_address: String::from("mc.hypixel.net"),
            mc_player_name_input: String::new(),
            mc_uuid_input: String::new(),
            bridge: GoBridge::load(),
        }
    }
}

pub fn show(app: &mut AmnesiaApp, ctx: &egui::Context) {
    if !app.github.visible { return; }
    let ws_path = app.ws.as_ref().map(|w| w.path.clone()).unwrap_or_default();

    egui::Window::new("GitHub")
        .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
        .resizable(false)
        .show(ctx, |ui| {
            // bridge indicator
            let bridge_ok = app.github.bridge.is_some();
            ui.label(egui::RichText::new(
                if bridge_ok { "Go Bridge: active" } else { "Go Bridge: not loaded" }
            ).size(11.0).color(if bridge_ok { Color32::from_rgb(86, 156, 214) } else { Color32::from_gray(150) }));

            ui.separator();

            // --- Auth ---
            ui.heading("Authorization");
            ui.horizontal(|ui| {
                ui.label("Token:");
                ui.add(egui::TextEdit::singleline(&mut app.github.token).password(true).hint_text("ghp_..."));
                if ui.button("Save").clicked() && !app.github.token.is_empty() {
                    let token = app.github.token.clone();
                    app.github.output = if let Some(ref b) = app.github.bridge {
                        b.auth_store(&token)
                    } else { "bridge not loaded".into() };
                }
                if ui.button("Check").clicked() {
                    app.github.output = if let Some(ref b) = app.github.bridge {
                        b.auth_check()
                    } else { "bridge not loaded".into() };
                }
                if ui.button("Clear").clicked() {
                    app.github.output = if let Some(ref b) = app.github.bridge {
                        b.auth_clear()
                    } else { "bridge not loaded".into() };
                }
            });
            if ui.button("List Repos").clicked() {
                app.github.output = if let Some(ref b) = app.github.bridge {
                    b.list_repos()
                } else { "bridge not loaded".into() };
            }

            ui.separator();

            // --- Network ---
            ui.heading("Network");
            if ui.button("Check Internet").clicked() {
                app.github.output = if let Some(ref b) = app.github.bridge {
                    b.net_check()
                } else { "bridge not loaded".into() };
            }
            ui.horizontal(|ui| {
                ui.label("DNS:");
                ui.text_edit_singleline(&mut app.github.dns_host);
                if ui.button("Lookup").clicked() {
                    let host = app.github.dns_host.clone();
                    app.github.output = if let Some(ref b) = app.github.bridge {
                        b.dns_lookup(&host)
                    } else { "bridge not loaded".into() };
                }
            });
            ui.horizontal(|ui| {
                ui.label("HTTP GET:");
                ui.text_edit_singleline(&mut app.github.http_url);
                if ui.button("Fetch").clicked() {
                    let url = app.github.http_url.clone();
                    app.github.output = if let Some(ref b) = app.github.bridge {
                        b.http_get(&url)
                    } else { "bridge not loaded".into() };
                }
            });

            ui.separator();

            // --- Clone ---
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

            if !ws_path.is_empty() {
            ui.separator();

            // --- Minecraft ---
            ui.heading("Minecraft");
            ui.horizontal(|ui| {
                ui.label("Server:");
                ui.text_edit_singleline(&mut app.github.mc_address);
                if ui.button("Ping").clicked() {
                    let addr = app.github.mc_address.clone();
                    app.github.output = if let Some(ref b) = app.github.bridge {
                        b.mc_ping_server(&addr)
                    } else { "bridge not loaded".into() };
                }
            });
            ui.horizontal(|ui| {
                ui.label("Player:");
                ui.text_edit_singleline(&mut app.github.mc_player_name_input);
                if ui.button("UUID").clicked() {
                    let name = app.github.mc_player_name_input.clone();
                    app.github.output = if let Some(ref b) = app.github.bridge {
                        b.mc_player_uuid(&name)
                    } else { "bridge not loaded".into() };
                }
            });
            ui.horizontal(|ui| {
                ui.label("UUID:");
                ui.text_edit_singleline(&mut app.github.mc_uuid_input);
                if ui.button("Name").clicked() {
                    let uuid = app.github.mc_uuid_input.clone();
                    app.github.output = if let Some(ref b) = app.github.bridge {
                        b.mc_player_name(&uuid)
                    } else { "bridge not loaded".into() };
                }
            });
            if ui.button("Mojang Status").clicked() {
                app.github.output = if let Some(ref b) = app.github.bridge {
                    b.mc_mojang_status()
                } else { "bridge not loaded".into() };
            }

            ui.separator();
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
