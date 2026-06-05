mod shell;
mod buffer;
pub mod render;

use std::sync::mpsc::{Sender, Receiver};

pub struct Term {
    #[allow(dead_code)]
    pub child: Option<std::process::Child>,
    pub output: String,
    pub input: String,
    tx: Option<Sender<String>>,
    rx: Option<Receiver<String>>,
}

impl Term {
    pub fn spawn(workdir: &str) -> Self {
        let (tx, rx, child) = shell::start(workdir);
        Term { child, output: String::new(), input: String::new(), tx, rx }
    }

    pub fn poll(&mut self) {
        buffer::drain(&mut self.output, &self.rx);
    }

    pub fn exec(&mut self, cmd: &str) {
        if let Some(tx) = &self.tx {
            let _ = tx.send(cmd.to_string());
        }
    }
}
