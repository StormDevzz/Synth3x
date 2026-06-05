use std::process::Command;
use std::str;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let mode = args.get(1).map(|s| s.as_str()).unwrap_or("daemon");

    match mode {
        "dhcp" => run_dhcp(),
        "daemon" => run_daemon(),
        "status" => print_status(),
        _ => eprintln!("usage: synit-svc <dhcp|daemon|status>"),
    }
}

fn run_dhcp() {
    let net = std::fs::read_dir("/sys/class/net");
    if let Ok(dir) = net {
        for entry in dir.flatten() {
            let name = entry.file_name();
            let n = name.to_string_lossy();
            if n == "." || n == ".." || n == "lo" || n.contains("docker") || n.contains("veth") {
                continue;
            }
            let _ = Command::new("dhcpcd").arg("-q").arg(n.as_ref()).status();
            let _ = Command::new("busybox")
                .args(["udhcpc", "-i", n.as_ref(), "-q"])
                .status();
        }
    }
}

fn run_daemon() {
    match unsafe { libc::fork() } {
        0 => {
            run_dhcp();
            std::process::exit(0);
        }
        -1 => eprintln!("svc: fork failed"),
        _ => {}
    }

    loop {
        std::thread::sleep(std::time::Duration::from_secs(30));
        check_network();
    }
}

fn check_network() {
    let ping = Command::new("busybox")
        .args(["ping", "-c1", "-W2", "1.1.1.1"])
        .status();

    match ping {
        Ok(s) if s.success() => {}
        _ => {
            run_dhcp();
        }
    }
}

fn print_status() {
    let pid = std::fs::read_to_string("/tmp/synit-svc.pid").unwrap_or_default();
    if !pid.trim().is_empty() {
        println!("synit-svc: running (pid {})", pid.trim());
    } else {
        println!("synit-svc: not running");
    }

    let dns = Command::new("busybox")
        .args(["ping", "-c1", "-W2", "1.1.1.1"])
        .status();
    match dns {
        Ok(s) if s.success() => println!("network: connected"),
        _ => println!("network: offline"),
    }
}
