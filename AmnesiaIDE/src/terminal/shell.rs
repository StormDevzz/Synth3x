use std::io::{Read, Write};
use std::process::{Command, Stdio};
use std::sync::mpsc::{Sender, Receiver, channel};
use std::thread;

pub fn start(workdir: &str) -> (Option<Sender<String>>, Option<Receiver<String>>, Option<std::process::Child>) {
    let (tx_out, rx_out) = channel();
    let (tx_in, rx_in) = channel();

    let child = Command::new("sh")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .current_dir(workdir)
        .spawn()
        .ok();

    if let Some(mut c) = child {
        let t1 = tx_out.clone();
        if let Some(mut s) = c.stdout.take() {
            thread::spawn(move || {
                let mut buf = [0u8; 4096];
                while let Ok(n) = s.read(&mut buf) {
                    if n == 0 { break; }
                    let _ = t1.send(String::from_utf8_lossy(&buf[..n]).to_string());
                }
            });
        }
        if let Some(mut s) = c.stderr.take() {
            thread::spawn(move || {
                let mut buf = [0u8; 4096];
                while let Ok(n) = s.read(&mut buf) {
                    if n == 0 { break; }
                    let _ = tx_out.send(String::from_utf8_lossy(&buf[..n]).to_string());
                }
            });
        }
        if let Some(mut w) = c.stdin.take() {
            thread::spawn(move || {
                while let Ok(cmd) = rx_in.recv() {
                    let _ = writeln!(w, "{}", cmd);
                    let _ = w.flush();
                }
            });
        }
        (Some(tx_in), Some(rx_out), c.into())
    } else {
        (None, None, None)
    }
}
