use egui::Color32;
use crate::syntax::lang::{Lang, is_keyword};
use crate::syntax::colors::{COMMENT, STRING, KEYWORD, NUMBER, PREPROC, BRACKET, TYPE};

pub fn tokenize(text: &str, lang: Lang) -> Vec<(usize, usize, Color32)> {
    let mut out: Vec<(usize, usize, Color32)> = Vec::new();
    let bytes = text.as_bytes();
    let len = bytes.len();
    let mut i = 0;

    while i < len {
        if i + 1 < len && bytes[i] == b'/' && bytes[i + 1] == b'/' {
            out.push((i, len, COMMENT));
            break;
        }
        if i + 1 < len && bytes[i] == b'/' && bytes[i + 1] == b'*' {
            let mut end = i + 2;
            while end + 1 < len && !(bytes[end] == b'*' && bytes[end + 1] == b'/') { end += 1; }
            if end + 1 < len { end += 2; }
            out.push((i, end, COMMENT));
            i = end; continue;
        }
        if bytes[i] == b'"' || bytes[i] == b'\'' {
            let q = bytes[i]; let start = i; i += 1;
            while i < len && bytes[i] != q {
                if bytes[i] == b'\\' && i + 1 < len { i += 2; } else { i += 1; }
            }
            if i < len { i += 1; }
            out.push((start, i, STRING)); continue;
        }
        if bytes[i] == b'#' {
            let start = i;
            while i < len && bytes[i] != b'\n' { i += 1; }
            out.push((start, i, PREPROC)); continue;
        }
        if bytes[i].is_ascii_digit() {
            let start = i;
            while i < len && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'.' || bytes[i] == b'x' || bytes[i] == b'X') { i += 1; }
            out.push((start, i, NUMBER)); continue;
        }
        if bytes[i].is_ascii_alphabetic() || bytes[i] == b'_' {
            let start = i;
            while i < len && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'_') { i += 1; }
            let word = &text[start..i];
            if is_keyword(word, lang) { out.push((start, i, KEYWORD)); }
            else if word.as_bytes()[0].is_ascii_uppercase() { out.push((start, i, TYPE)); }
            continue;
        }
        if matches!(bytes[i], b'{' | b'}' | b'(' | b')' | b'[' | b']') {
            out.push((i, i + 1, BRACKET)); i += 1; continue;
        }
        i += 1;
    }
    out
}
