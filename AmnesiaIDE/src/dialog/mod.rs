pub mod scaffold;

use eframe::egui;
use crate::editor::AmnesiaApp;
use std::collections::HashSet;

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum Lang {
    C, Cpp, Csharp, Rust, Go, Zig, Nim, Asm, Python, JavaScript, TypeScript, Lua, OCaml, Haskell
}

impl Lang {
    pub fn label(&self) -> &'static str {
        use Lang::*;
        match self {
            C => "C", Cpp => "C++", Csharp => "C#", Rust => "Rust",
            Go => "Go", Zig => "Zig", Nim => "Nim", Asm => "Assembly",
            Python => "Python", JavaScript => "JavaScript", TypeScript => "TypeScript",
            Lua => "Lua", OCaml => "OCaml", Haskell => "Haskell",
        }
    }
    pub fn ext(&self) -> &'static str {
        use Lang::*;
        match self {
            C => "c", Cpp => "cpp", Csharp => "cs", Rust => "rs",
            Go => "go", Zig => "zig", Nim => "nim", Asm => "asm",
            Python => "py", JavaScript => "js", TypeScript => "ts",
            Lua => "lua", OCaml => "ml", Haskell => "hs",
        }
    }
}

pub struct NewProjectState {
    pub visible: bool,
    pub name: String,
    pub langs: HashSet<Lang>,
    pub init_git: bool,
    pub github_url: String,
}

impl Default for NewProjectState {
    fn default() -> Self {
        let mut langs = HashSet::new();
        langs.insert(Lang::C);
        Self { visible: false, name: String::new(), langs, init_git: true, github_url: String::new() }
    }
}

pub fn show(app: &mut AmnesiaApp, ctx: &egui::Context) {
    if !app.new_project.visible { return; }
    let all_langs = &[
        Lang::C, Lang::Cpp, Lang::Csharp, Lang::Rust, Lang::Go,
        Lang::Zig, Lang::Python, Lang::JavaScript, Lang::TypeScript,
        Lang::Lua, Lang::Nim, Lang::OCaml, Lang::Haskell, Lang::Asm,
    ];

    egui::Window::new("New Project")
        .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
        .collapsible(false)
        .resizable(false)
        .show(ctx, |ui| {
            ui.horizontal(|ui| { ui.label("Name:"); ui.text_edit_singleline(&mut app.new_project.name); });
            ui.label(egui::RichText::new(format!("Location: ~/Amnesia/{}", app.new_project.name)).color(egui::Color32::from_gray(150)).size(12.0));
            ui.add_space(6.0);
            ui.label("What languages do you want to use?");
            egui::ScrollArea::vertical().max_height(160.0).show(ui, |ui| {
                ui.columns(4, |columns| {
                    for (i, l) in all_langs.iter().enumerate() {
                        let col = i % 4;
                        let mut checked = app.new_project.langs.contains(l);
                        columns[col].checkbox(&mut checked, l.label());
                        if checked { app.new_project.langs.insert(*l); }
                        else { app.new_project.langs.remove(l); }
                    }
                });
            });
            ui.add_space(6.0);
            ui.checkbox(&mut app.new_project.init_git, "Init git repository");
            ui.horizontal(|ui| {
                ui.label("GitHub URL:");
                ui.text_edit_singleline(&mut app.new_project.github_url);
            });
            ui.add_space(10.0);
            ui.horizontal(|ui| {
                if ui.button("Create Project").clicked() {
                    let name = app.new_project.name.trim().to_owned();
                    if !name.is_empty() && !app.new_project.langs.is_empty() {
                        let dir = format!("{}/{}", app.home, name);
                        let langs: Vec<Lang> = app.new_project.langs.iter().copied().collect();
                        scaffold::create(&dir, &name, &langs, app.new_project.init_git, &app.new_project.github_url);
                        app.open_workspace(&dir);
                        app.notify.push(crate::notifications::store::Level::Info, format!("Project '{}' created", name));
                        app.new_project.visible = false;
                    }
                }
                if ui.button("Cancel").clicked() { app.new_project.visible = false; }
            });
        });
}
