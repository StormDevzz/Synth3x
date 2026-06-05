#[derive(Debug, Clone, Copy)]
pub enum Level {
    Info,
    Warn,
    Error,
}

#[derive(Debug, Clone)]
pub struct Notification {
    pub level: Level,
    pub text: String,
    pub alive: u32, // frames remaining
}

pub const FRAMES: u32 = 300; // ~5 seconds at 60fps

pub struct Notify {
    pub items: Vec<Notification>,
}

impl Notify {
    pub fn new() -> Self { Notify { items: Vec::new() } }

    pub fn push(&mut self, level: Level, text: String) {
        self.items.push(Notification { level, text, alive: FRAMES });
    }

    pub fn tick(&mut self) {
        self.items.retain_mut(|n| {
            n.alive = n.alive.saturating_sub(1);
            n.alive > 0
        });
    }
}
