use std::fs;
use std::io::{self, Write};
use std::path::Path;
use std::process::Command;

use synth3x_safe::VERSION_TAG;

const OS_NAME: &str = "Synth3x";

/// ─── ANSI color constants ───
const HX: &str = "\x1b[0m";
const NEON_CYAN: &str = "\x1b[38;2;0;255;200m";
const NEON_PINK: &str = "\x1b[38;2;255;0;128m";
const NEON_PURPLE: &str = "\x1b[38;2;140;0;255m";
const NEON_RED: &str = "\x1b[38;2;255;30;60m";
const DARK_RED: &str = "\x1b[38;2;180;0;30m";
const NEON_GREEN: &str = "\x1b[38;2;0;255;100m";
const NEON_YELLOW: &str = "\x1b[38;2;255;220;40m";
const DIM: &str = "\x1b[38;2;80;80;100m";
const BOLD: &str = "\x1b[1m";
const FG: &str = "\x1b[38;2;180;180;200m";
const BG2: &str = "\x1b[48;2;15;12;22m";
const WARN_BG: &str = "\x1b[48;2;40;5;10m";
const CROSS: &str = "\x1b[38;2;255;30;60m";
const HINT: &str = "\x1b[38;2;120;160;255m";

/// ─── Termios for masked password input ───
#[cfg(unix)]
fn prompt_password(msg: &str) -> String {
    use std::os::unix::io::AsRawFd;
    let fd = io::stdin().as_raw_fd();
    let mut termios: libc::termios = unsafe { std::mem::zeroed() };
    let has_tty = unsafe { libc::tcgetattr(fd, &mut termios) == 0 };

    if has_tty {
        let orig = termios;
        termios.c_lflag &= !libc::ECHO;
        unsafe { libc::tcsetattr(fd, libc::TCSANOW, &termios); }

        print!("{}", msg);
        io::stdout().flush().ok();
        let mut input = String::new();
        io::stdin().read_line(&mut input).ok();
        println!();

        unsafe { libc::tcsetattr(fd, libc::TCSANOW, &orig); }
        input.trim().to_string()
    } else {
        print!("{}", msg);
        io::stdout().flush().ok();
        let mut input = String::new();
        io::stdin().read_line(&mut input).ok();
        input.trim().to_string()
    }
}

/// Accept y, yes, Y, YES
fn prompt_yes(msg: &str) -> bool {
    let r = prompt(msg);
    let t = r.trim().to_lowercase();
    t == "y" || t == "yes" || t == "YES"
}

fn clear_screen() {
    print!("\x1b[2J\x1b[H");
    io::stdout().flush().ok();
}

fn show_banner() {
    clear_screen();
    println!("{}", BG2);
    println!("{}     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓{}", NEON_CYAN, HX);
    println!("{}     ▓{}  {}███████╗██╗   ██╗███╗   ██╗████████╗██╗  ██╗██████╗ ██╗  ██╗{}{}  ▓{}",
        NEON_CYAN, HX, BG2, NEON_PURPLE, NEON_CYAN, HX);
    println!("{}     ▓{}  {}██╔════╝╚██╗ ██╔╝████╗  ██║╚══██╔══╝██║  ██║╚══██╔══╝╚██╗██╔╝{}{}  ▓{}",
        NEON_CYAN, HX, BG2, NEON_PURPLE, NEON_CYAN, HX);
    println!("{}     ▓{}  {}███████╗ ╚████╔╝ ██╔██╗ ██║   ██║   ███████║   ██║    ╚███╔╝ {}{}  ▓{}",
        NEON_CYAN, HX, BG2, NEON_PURPLE, NEON_CYAN, HX);
    println!("{}     ▓{}  {}╚════██║  ╚██╔╝  ██║╚██╗██║   ██║   ██╔══██║   ██║    ██╔██╗ {}{}  ▓{}",
        NEON_CYAN, HX, BG2, NEON_PURPLE, NEON_CYAN, HX);
    println!("{}     ▓{}  {}███████║   ██║   ██║ ╚████║   ██║   ██║  ██║   ██║   ██╔╝ ██╗{}{}  ▓{}",
        NEON_CYAN, HX, BG2, NEON_PURPLE, NEON_CYAN, HX);
    println!("{}     ▓{}  {}╚══════╝   ╚═╝   ╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝{}{}  ▓{}",
        NEON_CYAN, HX, BG2, NEON_PURPLE, NEON_CYAN, HX);
    println!("{}     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓{}", NEON_CYAN, HX);
    println!();
    println!("{}     ════════════════════════════════════════════════════════════{}", DIM, HX);
    println!("{}     ┃{}  {}SYNTH3X-ANON v{}   TERMINAL INSTALLER{}              {}┃{}",
        DIM, HX, NEON_CYAN, VERSION_TAG, HX, DIM, HX);
    println!("{}     ┃{}  {}Terminal-only  •  No GUI required{}               {}┃{}",
        DIM, HX, DIM, HX, DIM, HX);
    println!("{}     ┃{}  {}Portage/emerge  •  WiFi setup  •  Disk selection{}   {}┃{}",
        DIM, HX, DIM, HX, DIM, HX);
    println!("{}     ════════════════════════════════════════════════════════════{}", DIM, HX);
    println!();
}

fn show_hint(text: &str) {
    println!("     {}▸ HINT: {}{}", HINT, text, HX);
}

fn show_step(num: u8, total: u8, title: &str) {
    println!("     {}{}[{}/{}]{} {}{}", NEON_CYAN, BOLD, num, total, HX, FG, title);
    println!();
}

fn abort(msg: &str) -> ! {
    println!("\n{}╔══════════════════════════════════════════════════════════╗{}", CROSS, HX);
    println!("{}║{}  {} ABORTED:{}{} {}", CROSS, HX, CROSS, BOLD, HX, msg);
    println!("{}║{}  {}Installation cancelled. No changes made.{}", CROSS, HX, NEON_YELLOW, HX);
    println!("{}╚══════════════════════════════════════════════════════════╝{}", CROSS, HX);
    std::process::exit(1);
}

fn prompt(msg: &str) -> String {
    print!("{}", msg);
    io::stdout().flush().ok();
    let mut input = String::new();
    io::stdin().read_line(&mut input).ok();
    input.trim().to_string()
}

fn prompt_default(msg: &str, default: &str) -> String {
    let r = prompt(msg);
    if r.is_empty() { default.to_string() } else { r }
}

/// ─── Detecting boot disk ───
fn detect_boot_disk() -> Option<String> {
    if let Ok(cmdline) = fs::read_to_string("/proc/cmdline") {
        if let Some(root) = cmdline.split_whitespace()
            .find(|p| p.starts_with("root="))
            .and_then(|p| p.split('=').nth(1))
        {
            let dev: String = root.chars().take_while(|c| c.is_alphabetic() || *c == '/' || *c == 'v' || *c == 's' || *c == 'n' || *c == 'm' || *c == 'd').collect();
            if !dev.is_empty() && dev != root {
                return Some(dev);
            }
            return Some(root.trim_end_matches(char::is_numeric).to_string());
        }
    }
    if let Ok(mtab) = fs::read_to_string("/etc/mtab") {
        for line in mtab.lines() {
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() >= 2 && parts[1] == "/" {
                return Some(parts[0].trim_end_matches(char::is_numeric).to_string());
            }
        }
    }
    None
}

/// ─── Drive selection ───
fn scan_drives() -> Vec<(String, u64, bool)> {
    let mut drives = Vec::new();
    if let Ok(entries) = fs::read_dir("/sys/block") {
        for entry in entries.flatten() {
            let name = entry.file_name();
            let name_str = name.to_string_lossy().to_string();
            if name_str.starts_with("loop") || name_str.starts_with("ram") ||
               name_str.starts_with("dm-") || name_str.starts_with("zram") ||
               name_str.starts_with("sr") {
                continue;
            }
            let size_path = entry.path().join("size");
            let removable_path = entry.path().join("removable");
            let size = fs::read_to_string(&size_path).ok()
                .and_then(|s| s.trim().parse::<u64>().ok()).unwrap_or(0) * 512;
            let removable = fs::read_to_string(&removable_path).ok()
                .map(|s| s.trim() == "1").unwrap_or(false);
            drives.push((name_str, size, removable));
        }
    }
    drives.sort_by(|a, b| a.0.cmp(&b.0));
    drives
}

/// Get disk model/vendor from sysfs
fn get_disk_info(name: &str) -> String {
    let model_path = format!("/sys/block/{}/model", name);
    let vendor_path = format!("/sys/block/{}/device/vendor", name);
    let model = fs::read_to_string(&model_path).ok()
        .map(|s| s.trim().to_string())
        .unwrap_or_default();
    let vendor = fs::read_to_string(&vendor_path).ok()
        .map(|s| s.trim().to_string())
        .unwrap_or_default();
    if !model.is_empty() {
        format!("{} {}", vendor, model).trim().to_string()
    } else if name.starts_with("nvme") {
        "NVMe SSD".to_string()
    } else if name.starts_with("sd") {
        "SATA/USB disk".to_string()
    } else if name.starts_with("vd") {
        "Virtual disk".to_string()
    } else {
        "Unknown".to_string()
    }
}

fn format_size(bytes: u64) -> String {
    if bytes >= 1_000_000_000_000 {
        format!("{:.1}TB", bytes as f64 / 1_000_000_000_000.0)
    } else if bytes >= 1_000_000_000 {
        format!("{:.0}GB", bytes as f64 / 1_000_000_000.0)
    } else if bytes >= 1_000_000 {
        format!("{:.0}MB", bytes as f64 / 1_000_000.0)
    } else {
        format!("{}B", bytes)
    }
}

/// ─── Quick internet check ───
fn check_internet() -> bool {
    Command::new("ping")
        .args(["-c", "1", "-W", "3", "1.1.1.1"])
        .status().map(|s| s.success()).unwrap_or(false)
}

/// ─── Detect wireless interface ───
fn detect_wifi_iface() -> Option<String> {
    let out = Command::new("sh")
        .args(["-c", "iw dev 2>/dev/null | grep Interface | awk '{print $2}' | head -1"])
        .output().ok()
        .and_then(|o| String::from_utf8(o.stdout).ok())?;
    let name = out.trim().to_string();
    if name.is_empty() { None } else { Some(name) }
}

/// ─── Load network drivers, bring interfaces up, run DHCP ───
fn bring_up_network() {
    // Load network drivers
    Command::new("sh").args(["-c",
        "modprobe virtio_net 2>/dev/null; \
         modprobe virtio 2>/dev/null; \
         modprobe virtio_ring 2>/dev/null; \
         modprobe virtio_pci 2>/dev/null; \
         modprobe e1000 2>/dev/null; \
         modprobe e100 2>/dev/null; \
         modprobe r8169 2>/dev/null; \
         sleep 1"]).status().ok();

    // Bring all non-lo interfaces up and run DHCP
    Command::new("sh").args(["-c",
        "for iface in /sys/class/net/*; do \
           name=$(basename $iface); \
           [ \"$name\" = \"lo\" ] && continue; \
           ip link set $name up 2>/dev/null; \
           dhcpcd -q $name 2>/dev/null || udhcpc -i $name -b -q 2>/dev/null; \
         done"]).status().ok();
}

/// ─── Show network interfaces status ───
fn show_network_status() {
    let out = Command::new("sh")
        .args(["-c", "ip addr show 2>/dev/null | grep -E '^[0-9]:|inet ' | head -20"])
        .output().ok()
        .and_then(|o| String::from_utf8(o.stdout).ok())
        .unwrap_or_default();
    if !out.is_empty() {
        println!("     {}Interfaces:{}", DIM, HX);
        for line in out.lines() {
            println!("      {}", line.trim());
        }
    }
}

/// ─── Network setup (Ethernet auto, WiFi on demand, blocks) ───
fn setup_wifi() {
    println!("     {}Network setup...{}", NEON_CYAN, HX);

    // Step 1: load drivers and bring up network
    bring_up_network();
    std::thread::sleep(std::time::Duration::from_secs(2));

    // Step 2: check internet
    if check_internet() {
        println!("     {}✓ Internet OK{}", NEON_GREEN, HX);
        return;
    }

    // Step 3: show interface status and retry DHCP
    println!("     {}Waiting for DHCP...{}", NEON_CYAN, HX);
    show_network_status();
    bring_up_network();
    std::thread::sleep(std::time::Duration::from_secs(4));

    if check_internet() {
        println!("     {}✓ Internet OK (DHCP){}", NEON_GREEN, HX);
        return;
    }

    // Step 4: if no WiFi hardware, loop with DHCP
    let wifi_iface = detect_wifi_iface();
    if wifi_iface.is_none() {
        println!("     {}No internet and no WiFi hardware found.{}", NEON_RED, HX);
        println!("     {}  Retrying network...{}", DIM, HX);
        loop {
            bring_up_network();
            std::thread::sleep(std::time::Duration::from_secs(3));
            if check_internet() {
                println!("     {}✓ Internet OK{}", NEON_GREEN, HX);
                return;
            }
            show_network_status();
            println!("     {}  Retrying... (check cable or WiFi adapter){}", DIM, HX);
        }
    }

    // Step 5: WiFi hardware found
    let iface = wifi_iface.unwrap();
    println!("     {}WiFi: {}{}", NEON_GREEN, iface, HX);

    Command::new("sh").args(["-c", &format!(
        "killall wpa_supplicant 2>/dev/null; \
         ip link set {} up 2>/dev/null; \
         synth3x-fastscan >/dev/null 2>&1 &", iface)]).status().ok();

    loop {
        println!();
        if !prompt_yes(&format!(
            "     {}┃{} {}Connect to WiFi?{} [{}y/skip]{}: ",
            DIM, HX, FG, HX, NEON_CYAN, HX))
        {
            if check_internet() {
                println!("     {}» Internet OK via Ethernet{}", NEON_YELLOW, HX);
                return;
            }
            println!("     {}» No internet. Use WiFi or plug Ethernet.{}", NEON_RED, HX);
            continue;
        }

        let ssid = prompt(&format!(
            "     {}┃{} {}Network name:{} {}",
            DIM, HX, FG, HX, NEON_CYAN));

        let password = prompt_password(&format!(
            "     {}┃{} {}Password:{} {}",
            DIM, HX, FG, HX, NEON_PURPLE));

        print!("     {}Connecting...{}", NEON_CYAN, HX);
        std::io::stdout().flush().ok();

        let mut ok = false;
        for attempt in 1..=3 {
            if attempt > 1 {
                print!("\r     {}Retry {}/3...{}", NEON_YELLOW, attempt, HX);
                std::io::stdout().flush().ok();
            }

            ok = Command::new("sh")
                .args(["-c", &format!(
                    "killall wpa_supplicant dhcpcd 2>/dev/null; \
                     ip link set {} up 2>/dev/null; \
                     wpa_passphrase '{}' '{}' > /tmp/wpa.conf 2>/dev/null; \
                     wpa_supplicant -B -i {} -c /tmp/wpa.conf 2>/dev/null; \
                     sleep 2; \
                     dhcpcd -q {} 2>/dev/null || udhcpc -i {} -b -q 2>/dev/null; \
                     sleep 2; \
                     ping -c 1 -W 3 1.1.1.1 >/dev/null 2>&1",
                    iface, ssid, password, iface, iface, iface
                )])
                .status()
                .map(|s| s.success())
                .unwrap_or(false);

            if ok { break; }
            std::thread::sleep(std::time::Duration::from_secs(1));
        }

        if ok {
            println!("\r     {}✓ Connected to '{}'{}", NEON_GREEN, ssid, HX);
            return;
        }
        println!("\r     {}✗ Failed{}", NEON_RED, HX);
        show_network_status();
        show_hint("Check name and password, or use Ethernet.");
    }
}

/// ─── Drive safety check ───
fn check_drive_safety(drive: &str, boot_disk: Option<&str>) {
    let s = format!("{}{}", WARN_BG, FG);
    print!("{}", s);
    print!("     {0}╔══════════════════════════════════════════════════════╗{1}",
        CROSS, HX);
    print!("\n");
    print!("     {0}║{1}  {0}{2}{3}  ⚠  DANGER: TARGET DRIVE ANALYSIS  ⚠{1}          {0}║{1}",
        CROSS, HX, BOLD, NEON_RED);
    println!();
    println!("     {}╚══════════════════════════════════════════════════════╝{}", CROSS, HX);
    println!();

    if let Some(boot) = boot_disk {
        if drive == boot {
            println!("     {}{}》 BOOT DISK DETECTED: {}{}", NEON_RED, BOLD, drive, HX);
            println!("     {}  This drive is currently running the OS.{}", FG, HX);
            println!("     {}  Installing over it will cause system failure.{}", CROSS, HX);
            println!();
            let msg = String::from("     ") + NEON_YELLOW + BOLD + "> Type " + NEON_PINK + "YES" + HX + " " + NEON_YELLOW + "to confirm boot disk wipe:" + HX + " " + NEON_PINK;
            if !prompt_yes(&msg) {
                abort("Boot disk installation rejected.");
            }
        }
    }

    let mount_output = Command::new("mount").output().ok()
        .and_then(|o| String::from_utf8(o.stdout).ok())
        .unwrap_or_default();
    let drive_parts = format!("{} ", drive.replace("/dev/", ""));
    if mount_output.contains(&drive_parts) {
        println!("     {}》 Mounted partitions detected on {}{}", NEON_YELLOW, drive, HX);
        println!();
        let msg = String::from("     ") + NEON_YELLOW + BOLD + "> Type " + NEON_PINK + "YES" + HX + " " + NEON_YELLOW + "to overwrite:" + HX + " " + NEON_PINK;
        if !prompt_yes(&msg) {
            abort("Mounted partition overwrite rejected.");
        }
    }

    println!();
    print!("     {0}╔══════════════════════════════════════════════════════╗{1}",
        DARK_RED, HX);
    print!("\n");
    print!("     {0}║{1}  {0}{2}{3}  ☠  FINAL WARNING — IRREVERSIBLE  ☠{1}             {0}║{1}",
        DARK_RED, HX, NEON_RED, BOLD);
    println!();
    println!("     {}╠══════════════════════════════════════════════════════╣{}", DARK_RED, HX);
    println!("     {}║{}  {}Target:{}  {}", DARK_RED, HX, NEON_PINK, HX, drive);
    println!("     {}║{}  {}Action:{}  ALL DATA WILL BE DESTROYED", DARK_RED, HX, NEON_PINK, HX);
    println!("     {}║{}  {}Note:{}   This operation cannot be undone.", DARK_RED, HX, NEON_PINK, HX);
    println!("     {}╚══════════════════════════════════════════════════════╝{}", DARK_RED, HX);
    println!();
    let confirm = prompt(&(String::from("     ") + NEON_CYAN + "> Confirm by typing the full path " + NEON_PURPLE + drive + HX + " " + NEON_CYAN + ":" + HX + " "));
    if confirm != drive {
        abort("Confirmation mismatch. Aborting.");
    }

    println!();
    println!("     {}{}» Countdown to destruction... Ctrl+C to abort{}", NEON_RED, BOLD, HX);
    for i in (1..=5).rev() {
        print!("     {}{}.{}", DIM, i, HX);
        io::stdout().flush().ok();
        std::thread::sleep(std::time::Duration::from_secs(1));
        print!("\x1b[{}D{}", 6, " ".repeat(6));
        print!("\x1b[{}D", 6);
    }
    println!(" {}{} COMMENCING{}", CROSS, BOLD, HX);
    println!();
}

/// ─── Partitioning ───
fn partition_drive(drive: &str, simulation: bool) {
    println!("     {}{}⌛{} {}GPT partition table...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
    if !simulation {
        let status = Command::new("parted")
            .args(["-s", drive, "mklabel", "gpt"])
            .status();
        match status {
            Ok(s) if s.success() => println!("     {}✓{}", NEON_GREEN, HX),
            _ => abort("Failed to create partition table"),
        }

        println!("     {}{}⌛{} {}EFI partition (512MB)...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
        let status = Command::new("parted")
            .args(["-s", drive, "mkpart", "primary", "fat32", "1MiB", "513MiB"])
            .status();
        match status {
            Ok(_) => {
                Command::new("parted").args(["-s", drive, "set", "1", "esp", "on"]).status().ok();
            }
            _ => abort("EFI partition failed"),
        }

        println!("     {}{}⌛{} {}Root ext4...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
        let status = Command::new("parted")
            .args(["-s", drive, "mkpart", "primary", "ext4", "513MiB", "100%"])
            .status();
        match status {
            Ok(s) if s.success() => {
                Command::new("udevadm").arg("settle").status().ok();
                std::thread::sleep(std::time::Duration::from_secs(2));
            }
            _ => abort("Root partition failed"),
        }

        println!("     {}{}⌛{} {}Formatting...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
        let p1 = format!("{}1", drive);
        let p2 = format!("{}2", drive);
        let p1_alt = format!("{}p1", drive);
        let p2_alt = format!("{}p2", drive);

        let (efi_part, root_part) = if Path::new(&p1).exists() {
            (p1, p2)
        } else {
            (p1_alt, p2_alt)
        };

        Command::new("mkfs.vfat").args(["-F32", &efi_part]).status()
            .and_then(|s| if s.success() { Ok(()) } else { Err(std::io::Error::other("")) })
            .unwrap_or_else(|_| abort("FAT32 format failed"));

        Command::new("mkfs.ext4").args(["-F", &root_part]).status()
            .and_then(|s| if s.success() { Ok(()) } else { Err(std::io::Error::other("")) })
            .unwrap_or_else(|_| abort("ext4 format failed"));

        println!("     {}✓{}", NEON_GREEN, HX);
    } else {
        std::thread::sleep(std::time::Duration::from_secs(3));
    }
    println!();
    println!("     {}{}✓ PARTITIONING COMPLETE{}", NEON_GREEN, BOLD, HX);
    std::thread::sleep(std::time::Duration::from_secs(1));
}

/// ─── Download and install Stage3 via C downloader ───
fn download_stage3() -> bool {
    println!("     {}Downloading Gentoo Stage3 via C downloader...{}", NEON_CYAN, HX);

    let status = Command::new("/usr/bin/synth3x-downloader")
        .arg("--stage3")
        .status();

    match status {
        Ok(s) if s.success() => {
            println!("     {}✓ Stage3 downloaded and extracted{}", NEON_GREEN, HX);
            return true;
        }
        _ => {
            println!("     {}[WARN] C downloader not available, using wget fallback{}", NEON_YELLOW, HX);
        }
    }

    let stage3_url = "https://bouncer.gentoo.org/fetch/root/all/releases/amd64/autobuilds/current-stage3-amd64-openrc/stage3-amd64-openrc-latest.tar.xz";
    println!("     {}URL: {}{}", DIM, stage3_url, HX);

    let download = Command::new("wget")
        .args(["-q", "--show-progress", "-O", "/mnt/gentoo/stage3.tar.xz", stage3_url])
        .status()
        .or_else(|_| Command::new("curl")
            .args(["-L", "-o", "/mnt/gentoo/stage3.tar.xz", stage3_url])
            .status());

    match download {
        Ok(s) if s.success() => {
            println!("     {}» Extracting Stage3 tarball...{}", NEON_GREEN, HX);
            let extract = Command::new("tar")
                .args(["-xpf", "/mnt/gentoo/stage3.tar.xz", "-C", "/mnt/gentoo",
                       "--xattrs-include=*.*", "--numeric-owner"])
                .status();
            match extract {
                Ok(s) if s.success() => {
                    fs::remove_file("/mnt/gentoo/stage3.tar.xz").ok();
                    true
                }
                _ => {
                    println!("     {}[ERROR] Failed to extract Stage3{}", NEON_RED, HX);
                    false
                }
            }
        }
        _ => {
            println!("     {}[ERROR] Failed to download Stage3{}", NEON_RED, HX);
            false
        }
    }
}

/// ─── Install base system ───
fn install_base(drive: &str, username: &str, password: &str, hostname: &str,
                timezone: &str, locale: &str, simulation: bool) {
    let root_part = if Path::new(&format!("{}2", drive)).exists() {
        format!("{}2", drive)
    } else {
        format!("{}p2", drive)
    };

    if !simulation {
        println!("     {}{}⌛{} {}Mounting root partition...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
        fs::create_dir_all("/mnt/gentoo").ok();
        let status = Command::new("mount").args([&root_part, "/mnt/gentoo"]).status();
        match status {
            Ok(s) if s.success() => println!("     {}✓{}", NEON_GREEN, HX),
            _ => abort("Failed to mount root partition"),
        }

        println!("     {}》 Checking internet connection...{}", NEON_CYAN, HX);
        let online = Command::new("ping")
            .args(["-c", "1", "-W", "3", "gentoo.org"])
            .status().map(|s| s.success()).unwrap_or(false);

        if online {
            println!("     {}» Online. Downloading Gentoo Stage3...{}", NEON_GREEN, HX);
            if !download_stage3() {
                abort("Failed to download Stage3. Check internet connection.");
            }

            println!("     {}» Configuring base system...{}", NEON_GREEN, HX);
            fs::create_dir_all("/mnt/gentoo/proc").ok();
            fs::create_dir_all("/mnt/gentoo/sys").ok();
            fs::create_dir_all("/mnt/gentoo/dev").ok();

            Command::new("mount").args(["--bind", "/proc", "/mnt/gentoo/proc"]).status().ok();
            Command::new("mount").args(["--bind", "/sys", "/mnt/gentoo/sys"]).status().ok();
            Command::new("mount").args(["--bind", "/dev", "/mnt/gentoo/dev"]).status().ok();

            // Portage/emerge setup
            let make_conf = format!(
                "COMMON_FLAGS=\"-O2 -pipe -march=x86-64\"\n\
                 CFLAGS=\"${{COMMON_FLAGS}}\"\n\
                 CXXFLAGS=\"${{COMMON_FLAGS}}\"\n\
                 FCFLAGS=\"${{COMMON_FLAGS}}\"\n\
                 FFLAGS=\"${{COMMON_FLAGS}}\"\n\
                 PORTDIR=\"/var/db/repos/gentoo\"\n\
                 DISTDIR=\"/var/cache/distfiles\"\n\
                 PKGDIR=\"/var/cache/binpkgs\"\n\
                 LC_MESSAGES={locale}.utf8\n\
                 USE=\"wayland elogind dbus udev unicode -X -gnome -kde\"\n\
                 EMERGE_DEFAULT_OPTS=\"--ask=n --verbose --quiet --tree\"\n\
                 SYNC=\"https://rsync.gentoo.org/gentoo-portage\"\n"
            );
            fs::create_dir_all("/mnt/gentoo/etc/portage").ok();
            fs::write("/mnt/gentoo/etc/portage/make.conf", &make_conf).ok();

            fs::create_dir_all("/mnt/gentoo/etc/portage/package.env").ok();
            fs::create_dir_all("/mnt/gentoo/etc/portage/package.use").ok();

            // fstab
            let efi_part = if Path::new(&format!("{}1", drive)).exists() {
                format!("{}1", drive)
            } else {
                format!("{}p1", drive)
            };
            let fstab = format!(
                "{}   /boot       vfat    defaults,noatime    0 2\n{}   /           ext4    noatime             0 1\n",
                efi_part, root_part
            );
            fs::write("/mnt/gentoo/etc/fstab", &fstab).ok();
            fs::copy("/etc/resolv.conf", "/mnt/gentoo/etc/resolv.conf").ok();
            fs::write("/mnt/gentoo/etc/hostname", format!("{}\n", hostname)).ok();

            // Timezone
            if !timezone.is_empty() {
                fs::create_dir_all("/mnt/gentoo/etc").ok();
                fs::write("/mnt/gentoo/etc/timezone", format!("{}\n", timezone)).ok();
                let _ = Command::new("chroot")
                    .args(["/mnt/gentoo", "ln", "-sf",
                           &format!("/usr/share/zoneinfo/{}", timezone),
                           "/etc/localtime"])
                    .status();
            }

            // Locale
            if !locale.is_empty() {
                let locale_gen = format!("{}.UTF-8 UTF-8\n", locale);
                fs::write("/mnt/gentoo/etc/locale.gen", &locale_gen).ok();
                let _ = Command::new("chroot")
                    .args(["/mnt/gentoo", "locale-gen"])
                    .status();
                fs::write("/mnt/gentoo/etc/env.d/02locale",
                    format!("LANG={}.UTF-8\nLC_ALL={}.UTF-8\n", locale, locale)).ok();
            }

            // Create user
            Command::new("chroot")
                .args(["/mnt/gentoo", "useradd", "-m", "-G", "wheel,video,input,audio",
                       "-s", "/bin/bash", username])
                .status().ok();
            let pass_stdin = format!("{}:{}\n", username, password);
            if let Some(mut child) = Command::new("chroot")
                .args(["/mnt/gentoo", "chpasswd"])
                .stdin(std::process::Stdio::piped())
                .spawn().ok()
            {
                if let Some(mut stdin) = child.stdin.take() {
                    stdin.write_all(pass_stdin.as_bytes()).ok();
                }
                child.wait().ok();
            }
            let root_pass = format!("root:{}\n", password);
            if let Some(mut child) = Command::new("chroot")
                .args(["/mnt/gentoo", "chpasswd"])
                .stdin(std::process::Stdio::piped())
                .spawn().ok()
            {
                if let Some(mut stdin) = child.stdin.take() {
                    stdin.write_all(root_pass.as_bytes()).ok();
                }
                child.wait().ok();
            }

            // Sudo
            let sudo_entry = format!("{} ALL=(ALL) ALL\n", username);
            fs::create_dir_all("/mnt/gentoo/etc/sudoers.d").ok();
            fs::write("/mnt/gentoo/etc/sudoers.d/synth3x-user", &sudo_entry).ok();
            let perms = std::fs::Permissions::from_mode(0o440);
            fs::set_permissions("/mnt/gentoo/etc/sudoers.d/synth3x-user", perms).ok();
            let sudo_main = fs::read_to_string("/mnt/gentoo/etc/sudoers").unwrap_or_default();
            if !sudo_main.contains(&sudo_entry) {
                fs::write("/mnt/gentoo/etc/sudoers", format!("{}\n{}", sudo_main.trim(), sudo_entry)).ok();
            }

            // Autologin
            if Path::new("/mnt/gentoo/etc/inittab").exists() {
                let inittab = fs::read_to_string("/mnt/gentoo/etc/inittab").unwrap_or_default();
                let tty1_line = inittab.lines().find(|l| l.contains("tty1")).unwrap_or("");
                if !tty1_line.is_empty() {
                    let new_line = format!("c1:12345:respawn:/sbin/agetty --autologin {} --noclear 38400 tty1 linux", username);
                    let new_inittab = inittab.replace(tty1_line, &new_line);
                    fs::write("/mnt/gentoo/etc/inittab", &new_inittab).ok();
                }
            }

            let bash_profile = format!(
                "if [ -z \"$DISPLAY\" ] && [ \"$(tty)\" = \"/dev/tty1\" ]; then\n    exec /usr/bin/synth3x\nfi\n"
            );
            let home_dir = format!("/mnt/gentoo/home/{}", username);
            fs::create_dir_all(&home_dir).ok();
            fs::write(format!("{}/.bash_profile", home_dir), &bash_profile).ok();

            // Copy binaries
            for bin in &["synth3x", "syn", "ram_analyzer", "disk_analyzer",
                         "device_names", "usb_analyzer", "cable_analyzer",
                         "synth3x-installer", "synth3x-wifi", "synth3x-downloader"] {
                let src = format!("/usr/bin/{}", bin);
                let dst = format!("/mnt/gentoo/usr/bin/{}", bin);
                fs::copy(&src, &dst).ok();
            }

            fs::create_dir_all("/mnt/gentoo/etc").ok();
            fs::copy("/etc/nftables.rules", "/mnt/gentoo/etc/nftables.rules").ok();

            // Copy shared libs for compositor
            let ldd_output = Command::new("ldd").arg("/usr/bin/synth3x").output().ok()
                .and_then(|o| String::from_utf8(o.stdout).ok()).unwrap_or_default();
            for line in ldd_output.lines() {
                if let Some(path) = line.split_whitespace()
                    .find(|p| p.contains(".so") && p.starts_with('/')) {
                    if let Some(parent) = Path::new(path).parent() {
                        let _ = fs::create_dir_all(format!("/mnt/gentoo{}", parent.display()));
                        let _ = fs::copy(path, format!("/mnt/gentoo{}", path));
                    }
                }
            }

            Command::new("umount").arg("-l").arg("/mnt/gentoo/dev").status().ok();
            Command::new("umount").arg("-l").arg("/mnt/gentoo/sys").status().ok();
            Command::new("umount").arg("-l").arg("/mnt/gentoo/proc").status().ok();

            println!("     {}» Gentoo Base System built!{}", NEON_GREEN, HX);
        } else {
            println!("     {}» Offline. Copying host filesystem...{}", NEON_YELLOW, HX);
            let dirs = ["bin", "sbin", "usr", "etc", "var", "lib", "lib64"];
            for dir in &dirs {
                let src = format!("/{}", dir);
                let dst = format!("/mnt/gentoo/{}", dir);
                fs::create_dir_all(&dst).ok();
                if let Ok(entries) = fs::read_dir(&src) {
                    for entry in entries.flatten() {
                        let name = entry.file_name();
                        let src_path = entry.path();
                        let dst_path = Path::new(&dst).join(&name);
                        if src_path.is_dir() {
                            let _ = fs::create_dir_all(&dst_path);
                        } else {
                            let _ = fs::copy(&src_path, &dst_path);
                        }
                    }
                }
            }

            let _ = Command::new("useradd")
                .args(["-R", "/mnt/gentoo", "-m", "-G", "wheel", "-s", "/bin/bash", username])
                .status();
            if let Some(mut child) = Command::new("chpasswd")
                .args(["-R", "/mnt/gentoo"])
                .stdin(std::process::Stdio::piped())
                .spawn().ok()
            {
                if let Some(mut stdin) = child.stdin.take() {
                    let _ = stdin.write_all(format!("{}:{}\n", username, password).as_bytes());
                }
                child.wait().ok();
            }
        }

        fs::create_dir_all("/mnt/gentoo/proc").ok();
        fs::create_dir_all("/mnt/gentoo/sys").ok();
        fs::create_dir_all("/mnt/gentoo/dev").ok();
        fs::create_dir_all("/mnt/gentoo/tmp").ok();
        fs::create_dir_all("/mnt/gentoo/run").ok();

        Command::new("sync").status().ok();
        Command::new("umount").arg("/mnt/gentoo").status().ok();
    } else {
        std::thread::sleep(std::time::Duration::from_secs(3));
    }

    println!();
    println!("     {}{}✓ BASE SYSTEM INSTALLED{}", NEON_GREEN, BOLD, HX);
    std::thread::sleep(std::time::Duration::from_secs(1));
}

/// ─── DE Deployment ───
fn install_de(de_name: &str, simulation: bool) {
    println!("     {}Deploying {}...{}", NEON_CYAN, de_name, HX);
    if !simulation {
        match de_name {
            "AmnesiaDE (own compositor)" => {
                println!("     {}» AmnesiaDE compositor already in /usr/bin/synth3x{}", NEON_GREEN, HX);
                println!("     {}» Creating Wayland session entry...{}", DIM, HX);
                let _ = fs::create_dir_all("/mnt/gentoo/usr/share/wayland-sessions");
                fs::write("/mnt/gentoo/usr/share/wayland-sessions/synth3x.desktop",
                    "[Desktop Entry]\nName=AmnesiaDE\nComment=Synth3x Wayland Compositor\nExec=/usr/bin/synth3x\nType=Application\n").ok();
                println!("     {}» AmnesiaDE uses DRM/KMS Wayland protocol{}", DIM, HX);
                println!("     {}  No X11 required — native Wayland compositor{}", DIM, HX);
            }
            "KDE Plasma 6" => {
                println!("     {}» Installing KDE Plasma 6 via emerge...{}", NEON_GREEN, HX);
                let _ = Command::new("chroot")
                    .args(["/mnt/gentoo", "emerge", "--ask", "n", "--quiet",
                           "kde-plasma/plasma-meta"])
                    .status();
            }
            "GNOME Shell" => {
                println!("     {}» Installing GNOME Shell via emerge...{}", NEON_GREEN, HX);
                let _ = Command::new("chroot")
                    .args(["/mnt/gentoo", "emerge", "--ask", "n", "--quiet",
                           "gnome-base/gnome-shell", "gnome-base/gnome-control-center",
                           "gnome-base/gnome-session"])
                    .status();
            }
            _ => {}
        }
    } else {
        std::thread::sleep(std::time::Duration::from_secs(2));
    }
    println!();
    println!("     {}{}✓ DESKTOP ENVIRONMENT READY{}", NEON_GREEN, BOLD, HX);
    std::thread::sleep(std::time::Duration::from_secs(1));
}

/// ─── Bootloader ───
fn install_bootloader(drive: &str, simulation: bool) {
    let root_part = if Path::new(&format!("{}2", drive)).exists() {
        format!("{}2", drive)
    } else {
        format!("{}p2", drive)
    };
    let efi_part = if Path::new(&format!("{}1", drive)).exists() {
        format!("{}1", drive)
    } else {
        format!("{}p1", drive)
    };

    if !simulation {
        println!("     {}{}⌛{} {}Mounting partitions...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
        fs::create_dir_all("/mnt/gentoo").ok();
        Command::new("mount").args([&root_part, "/mnt/gentoo"]).status().ok();
        fs::create_dir_all("/mnt/gentoo/boot").ok();
        Command::new("mount").args([&efi_part, "/mnt/gentoo/boot"]).status().ok();
        println!("     {}✓{}", NEON_GREEN, HX);

        println!("     {}{}⌛{} {}Copying kernel + initramfs...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
        fs::copy("/boot/vmlinuz-linux", "/mnt/gentoo/boot/vmlinuz-linux").ok();
        fs::copy("/boot/initrd.img", "/mnt/gentoo/boot/initrd.img").ok();
        println!("     {}✓{}", NEON_GREEN, HX);

        println!("     {}{}⌛{} {}Installing GRUB...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
        let grub_status = Command::new("grub-install")
            .args(["--target=x86_64-efi", "--efi-directory=/mnt/gentoo/boot",
                   "--boot-directory=/mnt/gentoo/boot", "--removable", "--force"])
            .status();
        match grub_status {
            Ok(s) if s.success() => {}
            _ => {
                Command::new("grub-install")
                    .args(["--target=x86_64-efi", "--efi-directory=/mnt/gentoo/boot",
                           "--boot-directory=/mnt/gentoo/boot", "--removable", "--force",
                           "--modules=part_gpt fat ext2"])
                    .status().ok();
            }
        }
        println!("     {}✓{}", NEON_GREEN, HX);

        println!("     {}{}⌛{} {}GRUB config with Installer entry...{}", NEON_PURPLE, BOLD, HX, DIM, HX);
        fs::create_dir_all("/mnt/gentoo/boot/grub").ok();
        let grub_cfg = format!(
            "set timeout=5\n\
             set default=0\n\
             insmod all_video\n\
             insmod part_gpt\n\
             insmod fat\n\
             insmod ext2\n\
             \n\
             menuentry \"★ {} v{} (AmnesiaDE) ★\" {{\n\
             {}    linux /vmlinuz-linux loglevel=3 console=tty0\n\
             {}    initrd /initrd.img\n\
             }}\n\
             menuentry \"★ {} v{} Installer ★\" {{\n\
             {}    linux /vmlinuz-linux loglevel=3 console=tty0 installer\n\
             {}    initrd /initrd.img\n\
             }}\n\
             menuentry \"★ {} (Debug Mode) ★\" {{\n\
             {}    linux /vmlinuz-linux loglevel=7 console=tty0\n\
             {}    initrd /initrd.img\n\
             }}\n\
             menuentry \"Reboot\" {{ reboot }}\n\
             menuentry \"Shutdown\" {{ halt }}\n",
            OS_NAME, VERSION_TAG, "    ", "    ",
            OS_NAME, VERSION_TAG, "    ", "    ",
            OS_NAME, "    ", "    "
        );
        fs::write("/mnt/gentoo/boot/grub/grub.cfg", &grub_cfg).ok();
        println!("     {}✓{}", NEON_GREEN, HX);

        Command::new("sync").status().ok();
        Command::new("umount").args(["/mnt/gentoo/boot"]).status().ok();
        Command::new("umount").arg("/mnt/gentoo").status().ok();
    } else {
        std::thread::sleep(std::time::Duration::from_secs(3));
    }
    println!();
    println!("     {}{}✓ BOOTLOADER INSTALLED{}", NEON_GREEN, BOLD, HX);
    std::thread::sleep(std::time::Duration::from_secs(1));
}

fn show_complete(username: &str, drive: &str, hostname: &str, timezone: &str,
                 locale: &str, de_name: &str) {
    clear_screen();
    print!("{}", BG2);
    print!("{0}     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓{1}",
        NEON_CYAN, HX);
    print!("\n");
    print!("{0}     ▓{1}  {2}{3}██████╗ ██████╗ ███╗   ███╗██████╗ ██╗     ███████╗████████╗███████╗{0}{1}  ▓{1}",
        NEON_CYAN, HX, BG2, NEON_GREEN);
    print!("\n");
    print!("{0}     ▓{1}  {2}{3}██╔════╝██╔═══██╗████╗ ████║██╔══██╗██║     ██╔════╝╚══██╔══╝██╔════╝{0}{1}  ▓{1}",
        NEON_CYAN, HX, BG2, NEON_GREEN);
    print!("\n");
    print!("{0}     ▓{1}  {2}{3}██║     ██║   ██║██╔████╔██║██████╔╝██║     █████╗     ██║   █████╗{0}{1}  ▓{1}",
        NEON_CYAN, HX, BG2, NEON_GREEN);
    print!("\n");
    print!("{0}     ▓{1}  {2}{3}██║     ██║   ██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝     ██║   ██╔══╝{0}{1}  ▓{1}",
        NEON_CYAN, HX, BG2, NEON_GREEN);
    print!("\n");
    print!("{0}     ▓{1}  {2}{3}╚██████╗╚██████╔╝██║ ╚═╝ ██║██║     ███████╗███████╗   ██║   ███████╗{0}{1}  ▓{1}",
        NEON_CYAN, HX, BG2, NEON_GREEN);
    print!("\n");
    print!("{0}     ▓{1}  {2}{3}╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝   ╚═╝   ╚══════╝{0}{1}  ▓{1}",
        NEON_CYAN, HX, BG2, NEON_GREEN);
    print!("\n");
    print!("{0}     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓{1}",
        NEON_CYAN, HX);
    println!();
    println!("     {}{}  SYSTEM INSTALL COMPLETE{}", NEON_GREEN, BOLD, HX);
    println!();
    println!("     {}┌─────────────────────────────────────────────────────┐{}", DIM, HX);
    println!("     │  User:     {} {}", FG, username);
    println!("     │  Hostname: {} {}", FG, hostname);
    println!("     │  Drive:    {} {}", FG, drive);
    println!("     │  Timezone: {} {}", FG, timezone);
    println!("     │  Locale:   {} {}.UTF-8{}", FG, locale, HX);
    println!("     │  DE:       {} {}", FG, de_name);
    println!("     │  Version:  {} {}", FG, VERSION_TAG);
    println!("     │  Pkg:      {} emerge <package>{}", FG, HX);
    println!("     │  Wayland:  {} DRM/KMS (AmnesiaDE){}", FG, HX);
    println!("     {}└─────────────────────────────────────────────────────┘{}", DIM, HX);
    println!();
    println!("     {}{}》 REBOOT AND BOOT FROM DRIVE 《{}", NEON_PINK, BOLD, HX);
    println!("     {}  {} v{}  •  Rust + C + ASM  •  emerge  •  Wayland{}",
        DIM, OS_NAME, VERSION_TAG, HX);
    println!();
    println!("     {}HINT: In GRUB, select 'Installer' entry to re-enter setup{}", HINT, HX);
    println!();
}

use std::os::unix::fs::PermissionsExt;

fn main() {
    show_banner();
    println!("     {} {} BOOT.seq{} initializing", DIM, NEON_CYAN, HX);
    println!();

    // [1/10] Safety: boot disk detection
    show_step(1, 10, "Safety Check — Scanning for boot device...");
    let boot_disk = detect_boot_disk();
    match &boot_disk {
        Some(d) => println!("     {}{}》 {}{} {}flagged as boot disk{}", NEON_YELLOW, BOLD, d, HX, DIM, HX),
        None => println!("     {}{}》{} {}No boot disk — initramfs detected{}", NEON_GREEN, BOLD, HX, DIM, HX),
    }
    show_hint("Boot disk detection prevents accidental self-installation.");
    std::thread::sleep(std::time::Duration::from_secs(1));

    // [2/10] WiFi setup
    show_banner();
    show_step(2, 10, "Network Setup (WiFi/Ethernet)");
    setup_wifi();

    // [3/10] User setup
    show_banner();
    show_step(3, 10, "User Account Provisioning");
    let username = prompt_default(
        &format!("     {}┃{} {}Username:{} {}",
            DIM, HX, FG, HX, NEON_CYAN),
        "synth3x");
    println!("     {}┃ default →{} synth3x{}", DIM, HX, NEON_GREEN);
    show_hint("This user will have sudo access and auto-login on tty1.");
    show_hint("Username: lowercase letters, numbers, underscores only.");

    let password = prompt_password(
        &format!("     {}┃{} {}Password:{} {}",
            DIM, HX, FG, HX, NEON_PURPLE));
    let password2 = prompt_password(
        &format!("     {}┃{} {}Confirm:{} {}",
            DIM, HX, FG, HX, NEON_PURPLE));
    let password = if password == password2 && !password.is_empty() {
        password
    } else {
        if password != password2 {
            println!("     {}{} PASSWORDS DO NOT MATCH{}", CROSS, BOLD, HX);
            std::process::exit(1);
        }
        "synth3x".to_string()
    };
    println!();
    println!("     {0}┃{1} {2}{3}✓{1} {4}User:{5} {6}",
        DIM, HX, NEON_GREEN, HX, FG, NEON_CYAN, username);
    println!("     {0}┃{1} {2}{3}✓{1} {4}Sudo:{5} enabled",
        DIM, HX, NEON_GREEN, HX, FG, NEON_GREEN);
    std::thread::sleep(std::time::Duration::from_secs(1));

    // [4/10] Hostname + Timezone + Locale
    show_banner();
    show_step(4, 10, "System Configuration");

    let hostname = prompt_default(
        &format!("     {}┃{} {}Hostname:{} {}",
            DIM, HX, FG, HX, NEON_CYAN),
        "synth3x");
    println!("     {}┃ default →{} synth3x{}", DIM, HX, NEON_GREEN);
    show_hint("This is the machine name on the network.");

    let timezone = prompt_default(
        &format!("     {}┃{} {}Timezone:{} {}(e.g. Europe/Moscow, America/New_York, UTC){}",
            DIM, HX, FG, HX, DIM, HX),
        "UTC");
    println!("     {}┃ default →{} UTC{}", DIM, HX, NEON_GREEN);
    show_hint("Used for system clock. Full list: ls /usr/share/zoneinfo/");

    let locale = prompt_default(
        &format!("     {}┃{} {}Locale:{} {}(e.g. en_US, ru_RU, de_DE){}",
            DIM, HX, FG, HX, DIM, HX),
        "en_US");
    println!("     {}┃ default →{} en_US{}", DIM, HX, NEON_GREEN);
    show_hint("System language and character encoding. Will use UTF-8.");
    std::thread::sleep(std::time::Duration::from_secs(1));

    // [5/10] Drive scan
    show_banner();
    show_step(5, 10, "Storage Topology — Disk Selection");
    show_hint("Select the target disk for installation. ALL DATA on this drive will be ERASED.");
    show_hint("In QEMU with no disk, installer runs in simulation mode (safe to test).");

    let drives = scan_drives();
    let simulation = drives.is_empty();

    let drives = if simulation {
        println!("     {}{}》{} {}No physical disks — simulation mode{}", NEON_YELLOW, BOLD, HX, DIM, HX);
        vec![("vda".to_string(), 40_000_000_000u64, false)]
    } else {
        drives
    };

    println!();
    println!("     {}┌──────────────┬────────────┬──────────────────────────┐{}", DIM, HX);
    println!("     {}│{} {}DEVICE{}         {}│{} {}SIZE{}      {}│{} {}MODEL{}                      {}│{}",
        DIM, HX, NEON_CYAN, HX, DIM, HX, NEON_CYAN, HX, DIM, HX, NEON_CYAN, HX, DIM, HX);
    println!("     {}├──────────────┼────────────┼──────────────────────────┤{}", DIM, HX);
    for (i, (name, size, removable)) in drives.iter().enumerate() {
        let dev_path = format!("/dev/{}", name);
        let info = get_disk_info(name);
        let boot_flag = if let Some(boot) = &boot_disk {
            if dev_path == *boot || dev_path.starts_with(boot.as_str()) {
                format!(" {}(BOOT){}", NEON_RED, HX)
            } else { String::new() }
        } else { String::new() };
        let rem_flag = if *removable { " (USB)" } else { "" };
        println!("     {}│{} {}{}. /dev/{}{}{}  {}│{} {}  {}│{} {}  {}│{}",
            DIM, HX, NEON_PURPLE, i + 1, HX, name, boot_flag, rem_flag,
            DIM, HX, format_size(*size), DIM, HX, info, DIM);
    }
    println!("     {}└──────────────┴────────────┴──────────────────────────┘{}", DIM, HX);
    println!();
    let idx: usize = prompt_default(
        &format!("     {}Select target drive [{}{}1{}:{} {}]{}:{} {}",
            FG, NEON_PURPLE, HX, FG, HX, NEON_PURPLE, HX, NEON_CYAN, HX),
        "1")
        .parse()
        .unwrap_or(1);
    if idx < 1 || idx > drives.len() {
        abort("Invalid drive selection.");
    }
    let drive = format!("/dev/{}", drives[idx - 1].0);
    let drive_info = get_disk_info(&drives[idx - 1].0);
    println!("     {0}┃{1} {2}{3}✓{1} {4}Target:{5} {6} ({7})",
        DIM, HX, NEON_GREEN, HX, FG, NEON_CYAN, drive, drive_info);
    std::thread::sleep(std::time::Duration::from_secs(1));

    // [6/10] Safety check
    show_banner();
    show_step(6, 10, "Safety Verification Sequence");
    println!();
    if !simulation {
        check_drive_safety(&drive, boot_disk.as_deref());
    } else {
        println!("     {}» simulation — safety checks bypassed{}", DIM, HX);
        std::thread::sleep(std::time::Duration::from_secs(1));
    }

    // [7/10] DE selection
    show_banner();
    show_step(7, 10, "Desktop Environment Selection");
    println!("     {}┌─────────────────────────────────────────────────────┐{}", DIM, HX);
    println!("     {0}│{1}  {2}{3}1.{1} {4}{5}AmnesiaDE{1} {6} (own compositor, recommended){1}",
        DIM, HX, NEON_PURPLE, HX, NEON_CYAN, BOLD, DIM);
    println!("     {}│{}     {}cyberpunk DE  •  DRM/KMS  •  Wayland{}",
        DIM, HX, DIM, HX);
    println!("     {}│{}     {}No X11 — native Wayland protocol{}", DIM, HX, DIM, HX);
    println!("     {}│{}                                               {}│{}",
        DIM, HX, DIM, HX);
    println!("     {}│{}  {}{}2.{} {}KDE Plasma 6",
        DIM, HX, NEON_PURPLE, HX, FG, HX);
    println!("     {}│{}  {}{}3.{} {}GNOME Shell",
        DIM, HX, NEON_PURPLE, HX, FG, HX);
    println!("     {}└─────────────────────────────────────────────────────┘{}", DIM, HX);
    println!();
    show_hint("AmnesiaDE is the recommended choice — it's the custom Synth3x Wayland compositor.");
    show_hint("KDE/GNOME require internet for emerge packages (~1-2GB each).");
    println!();
    let de_choice = prompt_default(
        &format!("     {}Select [{}{}1{}:{} {}]{}{} {}",
            FG, NEON_PURPLE, HX, FG, HX, NEON_PURPLE, HX, NEON_CYAN, HX),
        "1");
    let de_name = match de_choice.as_str() {
        "1" => "AmnesiaDE (own compositor)",
        "2" => "KDE Plasma 6",
        "3" => "GNOME Shell",
        _ => abort("Invalid DE selection."),
    };
    println!("     {0}┃{1} {2}{3}✓{1} {4}Selected:{5} {6}",
        DIM, HX, NEON_GREEN, HX, FG, NEON_CYAN, de_name);
    std::thread::sleep(std::time::Duration::from_secs(1));

    // [8/10] Download files
    show_banner();
    show_step(8, 10, "Download Installation Files");
    show_hint("Stage3 + Portage will be downloaded from Gentoo mirrors. ~400MB total.");
    show_hint("If download fails, check internet: ping -c 1 1.1.1.1");
    println!();

    // [9/10] Partitioning
    show_banner();
    show_step(9, 10, "Partitioning & Formatting");
    println!("     {} {}{}", DIM, drive, HX);
    println!();
    show_hint("Creates GPT table: 512MB EFI (FAT32) + rest as root (ext4).");
    partition_drive(&drive, simulation);

    // [10/10] Base system + DE + Bootloader
    show_banner();
    show_step(10, 10, "Installing Base System + DE + Bootloader");
    println!();
    show_hint("Downloads Stage3 tarball, extracts to target, configures Portage/make.conf.");
    show_hint("Sets up fstab, hostname, timezone, locale, users, sudo, and autologin.");
    println!();
    install_base(&drive, &username, &password, &hostname, &timezone, &locale, simulation);

    show_banner();
    println!("     {}{}[10/10]{} {}Deploying {} + Bootloader{}", NEON_CYAN, BOLD, HX, FG, de_name, HX);
    println!();
    install_de(de_name, simulation);

    show_banner();
    println!("     {}{}[10/10]{} {}Bootloader Installation (UEFI){}", NEON_CYAN, BOLD, HX, FG, HX);
    println!();
    install_bootloader(&drive, simulation);

    // Complete
    show_complete(&username, &drive, &hostname, &timezone, &locale, de_name);
}
