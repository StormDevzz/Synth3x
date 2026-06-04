use eframe::egui::Visuals;
use crate::editor::{AmnesiaApp, views};

impl eframe::App for AmnesiaApp {
    fn update(&mut self, ctx: &eframe::egui::Context, _frame: &mut eframe::Frame) {
        ctx.set_visuals(Visuals::dark());

        match self.screen {
            Screen::Welcome => views::welcome(self, ctx),
            Screen::Workspace => views::workspace(self, ctx),
        }

        ctx.request_repaint();
    }
}

use crate::editor::Screen;
