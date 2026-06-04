use std::ffi::CString;
use std::io::{Error, ErrorKind};
use std::os::unix::process::CommandExt;
use std::process::Command;

extern "C" {
    fn geteuid() -> u32;
    fn setuid(uid: u32) -> i32;
    fn setgid(gid: u32) -> i32;
    fn initgroups(user: *const i8, gid: u32) -> i32;
}

#[derive(Debug, Clone, PartialEq)]
pub enum Privilege {
    Root,
    User,
    Unknown,
}

pub fn current_privilege() -> Privilege {
    let uid = unsafe { geteuid() };
    match uid {
        0 => Privilege::Root,
        _ => Privilege::User,
    }
}

pub fn drop_privileges(user: &str, group: Option<&str>) -> Result<(), Error> {
    let uid = lookup_user(user)?;
    let gid = match group {
        Some(g) => lookup_group(g)?,
        None => lookup_group(user).unwrap_or(uid),
    };

    let euid = unsafe { geteuid() };
    if euid != 0 {
        return Err(Error::new(ErrorKind::PermissionDenied,
            "not running as root, cannot drop privileges"));
    }

    let c_user = CString::new(user).map_err(|_| {
        Error::new(ErrorKind::InvalidInput, "username contains null byte")
    })?;

    let ret = unsafe { setgid(gid) };
    if ret != 0 {
        return Err(Error::last_os_error());
    }

    unsafe { initgroups(c_user.as_ptr(), gid) };

    let ret = unsafe { setuid(uid) };
    if ret != 0 {
        return Err(Error::last_os_error());
    }

    Ok(())
}

pub fn exec_as_user(cmd: &str, args: &[&str], user: &str) -> Error {
    let uid = lookup_user(user).unwrap_or(1000);
    let gid = lookup_group(user).unwrap_or(uid);

    let err = Command::new(cmd).args(args).uid(uid).gid(gid).exec();
    err
}

fn lookup_user(name: &str) -> Result<u32, Error> {
    let passwd = std::fs::read_to_string("/etc/passwd")
        .map_err(|e| Error::new(ErrorKind::NotFound, e))?;
    for line in passwd.lines() {
        let parts: Vec<&str> = line.split(':').collect();
        if parts.len() >= 3 && parts[0] == name {
            return parts[2].parse::<u32>()
                .map_err(|e| Error::new(ErrorKind::InvalidData, e));
        }
    }
    Err(Error::new(ErrorKind::NotFound,
        format!("user '{}' not found", name)))
}

fn lookup_group(name: &str) -> Result<u32, Error> {
    let group = std::fs::read_to_string("/etc/group")
        .map_err(|e| Error::new(ErrorKind::NotFound, e))?;
    for line in group.lines() {
        let parts: Vec<&str> = line.split(':').collect();
        if parts.len() >= 3 && parts[0] == name {
            return parts[2].parse::<u32>()
                .map_err(|e| Error::new(ErrorKind::InvalidData, e));
        }
    }
    Err(Error::new(ErrorKind::NotFound,
        format!("group '{}' not found", name)))
}
