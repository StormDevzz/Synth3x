use std::ffi::CString;
use std::os::raw::c_char;

extern "C" {
    fn term_raw_mode(enable: i32);
    fn term_get_size() -> TermSize;
    fn term_hide_cursor(hide: i32);
    fn term_write(s: *const c_char, len: i32);
    fn syntax_match(line: *const c_char, col: i32, out_color: *mut *const c_char) -> i32;
    fn syntax_lang(filename: *const c_char) -> i32;
    fn read_stdin_raw() -> i32;
}

#[repr(C)]
pub struct TermSize { pub rows: i32, pub cols: i32 }

pub fn raw_mode(on: bool) { unsafe { term_raw_mode(on as i32); } }
pub fn hide_cursor(hide: bool) { unsafe { term_hide_cursor(hide as i32); } }

pub fn get_size() -> (i32, i32) {
    unsafe { let ts = term_get_size(); (ts.rows, ts.cols) }
}

pub fn write_str(s: &str) {
    let bytes = s.as_bytes();
    unsafe { term_write(bytes.as_ptr() as *const c_char, bytes.len() as i32); }
}

pub fn get_syntax_color(line: &str, col: i32) -> Option<&'static str> {
    let c_line = CString::new(line).ok()?;
    let mut out: *const c_char = std::ptr::null();
    if unsafe { syntax_match(c_line.as_ptr(), col, &mut out) } != 0 && !out.is_null() {
        Some(unsafe { std::ffi::CStr::from_ptr(out).to_str().unwrap_or("") })
    } else {
        None
    }
}

pub fn detect_lang(path: &str) -> i32 {
    if let Ok(c) = CString::new(path) {
        unsafe { syntax_lang(c.as_ptr()) }
    } else { 0 }
}

pub fn read_key() -> i32 {
    unsafe { read_stdin_raw() }
}
