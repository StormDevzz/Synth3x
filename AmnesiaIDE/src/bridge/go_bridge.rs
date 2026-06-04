use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use serde::Deserialize;

#[derive(Deserialize)]
struct GhResult {
    out: String,
    err: Option<String>,
    info: Option<String>,
}

pub struct GoBridge {
    lib: libloading::Library,
}

impl GoBridge {
    pub fn load() -> Option<Self> {
        let paths = ["go/gh.so"];
        for p in &paths {
            if std::path::Path::new(p).exists() {
                if let Ok(lib) = unsafe { libloading::Library::new(p) } {
                    return Some(Self { lib });
                }
            }
        }
        None
    }

    unsafe fn call0(&self, name: &[u8]) -> Option<String> {
        let f: libloading::Symbol<unsafe extern "C" fn() -> *mut c_char> =
            self.lib.get(name).ok()?;
        let p = f();
        let s = CStr::from_ptr(p).to_string_lossy().to_string();
        if let Ok(free) = self.lib.get::<unsafe extern "C" fn(*mut c_char)>(b"gh_free") {
            free(p);
        }
        Some(s)
    }

    unsafe fn call1(&self, name: &[u8], arg: &str) -> Option<String> {
        let f: libloading::Symbol<unsafe extern "C" fn(*const c_char) -> *mut c_char> =
            self.lib.get(name).ok()?;
        let c = CString::new(arg).ok()?;
        let p = f(c.as_ptr());
        let s = CStr::from_ptr(p).to_string_lossy().to_string();
        if let Ok(free) = self.lib.get::<unsafe extern "C" fn(*mut c_char)>(b"gh_free") {
            free(p);
        }
        Some(s)
    }

    unsafe fn call2(&self, name: &[u8], a1: &str, a2: &str) -> Option<String> {
        let f: libloading::Symbol<
            unsafe extern "C" fn(*const c_char, *const c_char) -> *mut c_char,
        > = self.lib.get(name).ok()?;
        let c1 = CString::new(a1).ok()?;
        let c2 = CString::new(a2).ok()?;
        let p = f(c1.as_ptr(), c2.as_ptr());
        let s = CStr::from_ptr(p).to_string_lossy().to_string();
        if let Ok(free) = self.lib.get::<unsafe extern "C" fn(*mut c_char)>(b"gh_free") {
            free(p);
        }
        Some(s)
    }

    fn parse(json: &str) -> String {
        if let Ok(r) = serde_json::from_str::<GhResult>(json) {
            let mut lines = Vec::new();
            if let Some(info) = r.info {
                lines.push(info);
            }
            if !r.out.is_empty() {
                lines.push(r.out);
            }
            if let Some(e) = r.err {
                lines.push(e);
            }
            lines.join("\n")
        } else {
            json.to_owned()
        }
    }

    // --- auth ---
    pub fn auth_store(&self, token: &str) -> String {
        unsafe { self.call1(b"gh_auth_store", token) }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    pub fn auth_check(&self) -> String {
        unsafe { self.call0(b"gh_auth_check") }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    pub fn auth_clear(&self) -> String {
        unsafe { self.call0(b"gh_auth_clear") }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    pub fn list_repos(&self) -> String {
        unsafe { self.call0(b"gh_list_repos") }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    // --- network ---
    pub fn net_check(&self) -> String {
        unsafe { self.call0(b"gh_net_check") }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    pub fn dns_lookup(&self, host: &str) -> String {
        unsafe { self.call1(b"gh_dns_lookup", host) }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    pub fn http_get(&self, url: &str) -> String {
        unsafe { self.call1(b"gh_http_get", url) }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    // --- git ---
    pub fn clone_repo(&self, url: &str, dir: &str) -> String {
        unsafe { self.call2(b"gh_clone", url, dir) }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    pub fn status(&self, dir: &str) -> String {
        unsafe { self.call1(b"gh_status", dir) }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    pub fn commit_push(&self, dir: &str, msg: &str) -> String {
        unsafe { self.call2(b"gh_commit_push", dir, msg) }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }

    pub fn pull(&self, dir: &str) -> String {
        unsafe { self.call1(b"gh_pull", dir) }
            .map(|j| Self::parse(&j))
            .unwrap_or_else(|| "bridge error".into())
    }
}
