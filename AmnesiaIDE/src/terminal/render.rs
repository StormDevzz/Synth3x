use eframe::egui::{self, Color32};
use crate::terminal::Term;

pub fn show(ui: &mut egui::Ui, term: &mut Term) {
    ui.horizontal(|ui| {
        ui.label(egui::RichText::new("TERMINAL").color(Color32::from_rgb(86, 156, 214)).size(12.0));
    });
    egui::ScrollArea::vertical().stick_to_bottom(true).show(ui, |ui| {
        ui.set_min_height(60.0);
        ui.add_sized([ui.available_width(), 60.0], |ui: &mut egui::Ui| {
            let out = if term.output.is_empty() { " $ ".to_owned() } else { term.output.clone() };
            ui.label(egui::RichText::new(&out)
                .font(egui::FontId::new(13.0, egui::FontFamily::Monospace))
                .color(Color32::from_gray(200)));
            ui.allocate_rect(ui.available_rect_before_wrap(), egui::Sense::hover())
        });
    });
    ui.horizontal(|ui| {
        ui.label(egui::RichText::new("$").color(Color32::from_rgb(86, 156, 214)));
        let enter = ui.text_edit_singleline(&mut term.input).lost_focus()
            && ui.input(|i| i.key_pressed(egui::Key::Enter));
        if enter {
            let cmd = term.input.trim().to_owned();
            term.input.clear();
            if !cmd.is_empty() {
                term.output.push_str(&format!("\n$ {}", cmd));
                term.exec(&cmd);
            }
        }
    });
}
