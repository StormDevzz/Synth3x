use egui::Color32;

pub const COMMENT: Color32 = Color32::from_gray(128);
pub const STRING:  Color32 = Color32::from_rgb(206, 145, 120);
pub const KEYWORD: Color32 = Color32::from_rgb(86, 156, 214);
pub const NUMBER:  Color32 = Color32::from_rgb(181, 206, 168);
pub const PREPROC: Color32 = Color32::from_rgb(106, 153, 85);
pub const BRACKET: Color32 = Color32::from_rgb(255, 215, 0);
pub const TYPE:    Color32 = Color32::from_rgb(78, 201, 176);

pub fn default_fmt() -> egui::TextFormat {
    egui::TextFormat { font_id: egui::FontId::new(14.0, egui::FontFamily::Monospace), ..Default::default() }
}

pub fn colored_fmt(color: Color32) -> egui::TextFormat {
    egui::TextFormat { font_id: egui::FontId::new(14.0, egui::FontFamily::Monospace), color, ..Default::default() }
}
