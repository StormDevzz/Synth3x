use std::collections::HashMap;
use std::sync::Mutex;
use eframe::egui::{self, Color32, Frame, Rounding, Stroke, Pos2};
use crate::notifications::store::Notify;
use crate::notifications::store::Level;

static NOTIF_POS: Mutex<Option<HashMap<String, Pos2>>> = Mutex::new(None);

fn positions() -> std::sync::MutexGuard<'static, Option<HashMap<String, Pos2>>> {
    NOTIF_POS.lock().unwrap()
}

pub fn show(ctx: &egui::Context, notify: &mut Notify) {
    notify.tick();
    let mut pos_guard = positions();
    let pos_map = pos_guard.get_or_insert_with(HashMap::new);
    let mut y = 60.0;

    for n in &notify.items {
        let color = match n.level {
            Level::Info  => Color32::from_rgb(86, 156, 214),
            Level::Warn  => Color32::from_rgb(255, 193, 7),
            Level::Error => Color32::from_rgb(244, 67, 54),
        };
        let key = format!("notif_{}", y as i32);
        let text = format!(" {} {}", crate::icon::notif::for_level(n.level), n.text);
        let default = Pos2::new(ctx.screen_rect().right() - 10.0, y);

        let area_id = key.clone().into();
        let start_pos = pos_map.get(&key).copied().unwrap_or(default);

        egui::Area::new(area_id)
            .default_pos(start_pos)
            .movable(true)
            .show(ctx, |ui| {
                let r = ui.min_rect();
                pos_map.insert(key, r.min);

                Frame::none()
                    .fill(Color32::from_rgb(40, 38, 50))
                    .stroke(Stroke::new(1.0, color))
                    .rounding(Rounding::same(4.0))
                    .show(ui, |ui| {
                        ui.label(egui::RichText::new(&text).color(Color32::WHITE).size(13.0));
                    });
            });
        y += 28.0;
    }
}
