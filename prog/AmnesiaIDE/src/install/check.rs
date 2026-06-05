use std::process::Command;
use crate::install::compilers::{Compiler, ALL};
use crate::install::status::Status;
use crate::notifications::{Notify, Level};

pub fn check_all(notify: &mut Notify) -> Vec<(&'static Compiler, Status)> {
    let mut results = Vec::new();
    for c in ALL {
        let s = check_one(c);
        match &s {
            Status::Found { .. } => notify.push(Level::Info, format!("{}: OK", c.name)),
            Status::NotFound => notify.push(Level::Warn, format!("{} not found. Install: {}", c.name, c.install_hint)),
            Status::Error(e) => notify.push(Level::Error, format!("{}: {}", c.name, e)),
        }
        results.push((c, s));
    }
    results
}

fn check_one(c: &Compiler) -> Status {
    let out = Command::new(c.bin)
        .arg(c.version_flag)
        .output();
    match out {
        Ok(o) if o.status.success() => {
            let ver = String::from_utf8_lossy(&o.stdout)
                .lines().next().unwrap_or("?")
                .to_owned();
            if ver.is_empty() {
                Status::Found { version: c.bin.to_owned() }
            } else {
                Status::Found { version: ver }
            }
        }
        Ok(_) => Status::NotFound,
        Err(e) => Status::Error(e.to_string()),
    }
}
