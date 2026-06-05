use eframe::egui;

#[allow(dead_code)]
pub struct Picker {
    pub visible: bool,
    pub path: String,
    pub title: &'static str,
}

#[allow(dead_code)]
impl Picker {
    pub fn new(title: &'static str) -> Self {
        Picker { visible: true, path: String::new(), title }
    }

    pub fn show<F: FnMut(&str)>(&mut self, ctx: &egui::Context, mut f: F) {
        if !self.visible { return; }
        egui::Window::new(self.title)
            .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
            .collapsible(false)
            .resizable(false)
            .show(ctx, |ui| {
                ui.horizontal(|ui| { ui.label("Name:"); ui.text_edit_singleline(&mut self.path); });
                ui.horizontal(|ui| {
                    if ui.button(self.title).clicked() {
                        let p = self.path.trim().to_owned();
                        if !p.is_empty() { f(&p); }
                        self.visible = false;
                    }
                    if ui.button("Cancel").clicked() { self.visible = false; }
                });
            });
    }
}
