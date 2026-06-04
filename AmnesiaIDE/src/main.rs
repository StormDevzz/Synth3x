mod editor;
mod terminal;
mod file;
mod syntax;

fn main() -> Result<(), eframe::Error> {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([1100.0, 750.0])
            .with_min_inner_size([600.0, 400.0])
            .with_title("AmnesiaIDE"),
        ..Default::default()
    };
    eframe::run_native(
        "AmnesiaIDE",
        options,
        Box::new(|_cc| Box::<editor::AmnesiaApp>::default()),
    )
}
