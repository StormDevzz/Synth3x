use std::fs;
use std::path::Path;

/// Safe hardware detection wrappers.
/// These read from sysfs and /proc — no direct HW access, so they're safe.

#[derive(Debug, Clone)]
pub struct CpuInfo {
    pub vendor: String,
    pub model: String,
    pub cores: u32,
}

#[derive(Debug, Clone)]
pub struct MemoryInfo {
    pub total_kb: u64,
    pub available_kb: u64,
}

#[derive(Debug, Clone)]
pub struct DiskInfo {
    pub name: String,
    pub size_bytes: u64,
    pub is_removable: bool,
}

pub fn detect_cpu() -> CpuInfo {
    let vendor = read_first_line("/proc/cpuinfo", "vendor_id")
        .unwrap_or_else(|| "unknown".into());
    let model = read_first_line("/proc/cpuinfo", "model name")
        .unwrap_or_else(|| "unknown".into());
    let cores = count_occurrences("/proc/cpuinfo", "processor");
    CpuInfo { vendor, model, cores }
}

pub fn detect_memory() -> MemoryInfo {
    let total = read_meminfo("MemTotal").unwrap_or(0);
    let avail = read_meminfo("MemAvailable").unwrap_or(0);
    MemoryInfo { total_kb: total, available_kb: avail }
}

pub fn detect_disks() -> Vec<DiskInfo> {
    let mut disks = Vec::new();
    let sys_block = Path::new("/sys/block");
    if !sys_block.is_dir() {
        return disks;
    }
    if let Ok(entries) = fs::read_dir(sys_block) {
        for entry in entries.flatten() {
            let name = entry.file_name();
            let name_str = name.to_string_lossy();
            // Skip loop, ram, dm, zram devices
            if name_str.starts_with("loop") || name_str.starts_with("ram") ||
               name_str.starts_with("dm-") || name_str.starts_with("zram") ||
               name_str.starts_with("sr") {
                continue;
            }
            let size_path = entry.path().join("size");
            let removable_path = entry.path().join("removable");
            let size_bytes = read_u64(&size_path).unwrap_or(0) * 512;
            let is_removable = read_u64(&removable_path).unwrap_or(0) == 1;
            disks.push(DiskInfo {
                name: name_str.to_string(),
                size_bytes,
                is_removable,
            });
        }
    }
    disks.sort_by(|a, b| a.name.cmp(&b.name));
    disks
}

/// Returns true if running inside a VM (basic check).
pub fn is_vm() -> bool {
    let cpuinfo = fs::read_to_string("/proc/cpuinfo").unwrap_or_default();
    cpuinfo.to_lowercase().contains("hypervisor")
}

pub fn has_drm() -> bool {
    Path::new("/dev/dri/card0").exists()
}

pub fn has_fbdev() -> bool {
    Path::new("/dev/fb0").exists()
}

fn read_first_line(path: &str, prefix: &str) -> Option<String> {
    let content = fs::read_to_string(path).ok()?;
    for line in content.lines() {
        if let Some(val) = line.strip_prefix(prefix) {
            if let Some(v) = val.strip_prefix(':') {
                return Some(v.trim().to_string());
            }
        }
    }
    None
}

fn count_occurrences(path: &str, prefix: &str) -> u32 {
    let content = match fs::read_to_string(path) {
        Ok(c) => c,
        Err(_) => return 0,
    };
    content.lines().filter(|l| l.starts_with(prefix)).count() as u32
}

fn read_u64(path: &Path) -> Option<u64> {
    let s = fs::read_to_string(path).ok()?;
    s.trim().parse().ok()
}

fn read_meminfo(key: &str) -> Option<u64> {
    let content = fs::read_to_string("/proc/meminfo").ok()?;
    for line in content.lines() {
        if let Some(val) = line.strip_prefix(key) {
            if let Some(v) = val.strip_prefix(':') {
                let num: String = v.chars().filter(|c| c.is_ascii_digit()).collect();
                return num.parse().ok();
            }
        }
    }
    None
}
