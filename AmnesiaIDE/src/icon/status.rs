use crate::install::status::Status;

pub fn for_status(st: &Status) -> &'static str {
    match st {
        Status::Found { .. } => "✔",
        Status::NotFound => "✖",
        Status::Error(_) => "⚠",
    }
}
