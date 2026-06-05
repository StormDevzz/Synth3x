use crate::notifications::store::Level;

pub fn for_level(l: Level) -> &'static str {
    match l {
        Level::Info  => "ℹ",
        Level::Warn  => "⚠",
        Level::Error => "✖",
    }
}
