mod lang;
mod tokens;
mod colors;

pub use lang::{Lang, detect_lang};
use colors::{default_fmt, colored_fmt};
use egui::text::{LayoutJob, TextWrapping, LayoutSection};

pub fn highlight_job(text: &str, lang: Lang) -> LayoutJob {
    let sections = tokens::tokenize(text, lang);
    let bytes = text.len();

    let mut job = LayoutJob {
        text: text.to_owned(),
        wrap: TextWrapping { max_width: f32::INFINITY, max_rows: usize::MAX, break_anywhere: false, overflow_character: None },
        ..Default::default()
    };

    let mut pos = 0;
    for (start, end, color) in sections {
        if start > pos {
            job.sections.push(LayoutSection {
                leading_space: 0.0,
                byte_range: pos..start,
                format: default_fmt(),
            });
        }
        job.sections.push(LayoutSection {
            leading_space: 0.0,
            byte_range: start..end,
            format: colored_fmt(color),
        });
        pos = end;
    }
    if pos < bytes {
        job.sections.push(LayoutSection {
            leading_space: 0.0,
            byte_range: pos..bytes,
            format: default_fmt(),
        });
    }
    job
}
