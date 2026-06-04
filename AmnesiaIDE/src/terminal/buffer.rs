use std::sync::mpsc::Receiver;

pub fn drain(output: &mut String, rx: &Option<Receiver<String>>) {
    if let Some(rx) = rx {
        while let Ok(t) = rx.try_recv() {
            output.push_str(&t);
            if output.len() > 200_000 {
                let n = output.len() - 100_000;
                output.drain(..n);
            }
        }
    }
}
