use egui::{Color32, FontId, FontFamily, TextFormat};
use egui::text::{LayoutJob, TextWrapping, LayoutSection};

#[derive(Clone, Copy, PartialEq)]
pub enum Lang { None, C, Cpp, Csharp, Rust, Asm }

pub fn detect_lang(filename: &str) -> Lang {
    let ext = filename.rsplit('.').next().unwrap_or("");
    match ext {
        "c" | "h"            => Lang::C,
        "cpp" | "cc" | "cxx" | "hpp" | "hxx" => Lang::Cpp,
        "cs"                 => Lang::Csharp,
        "rs"                 => Lang::Rust,
        "asm" | "s" | "S" | "inc" => Lang::Asm,
        _                    => Lang::None,
    }
}

const KW_C: &[&str] = &[
    "auto","break","case","char","const","continue","default","do",
    "double","else","enum","extern","float","for","goto","if",
    "int","long","register","return","short","signed","sizeof","static",
    "struct","switch","typedef","union","unsigned","void","volatile","while",
];
const KW_CPP: &[&str] = &[
    "class","public","private","protected","virtual","override","explicit",
    "template","typename","namespace","using","this","new","delete",
    "friend","operator","inline","constexpr","nullptr","noexcept",
    "throw","catch","try","dynamic_cast","static_cast","reinterpret_cast",
    "const_cast","decltype","auto","enum","struct","union","extern",
    "mutable","volatile","export","alignas","alignof","static_assert",
];
const KW_CS: &[&str] = &[
    "abstract","as","base","bool","break","byte","case","catch","char",
    "checked","class","const","continue","decimal","default","delegate",
    "do","double","else","enum","event","explicit","extern","false",
    "finally","fixed","float","for","foreach","goto","if","implicit",
    "in","int","interface","internal","is","lock","long","namespace",
    "new","null","object","operator","out","override","params","private",
    "protected","public","readonly","ref","return","sbyte","sealed",
    "short","sizeof","stackalloc","static","string","struct","switch",
    "this","throw","true","try","typeof","uint","ulong","unchecked",
    "unsafe","ushort","using","var","virtual","void","volatile","while",
];
const KW_RUST: &[&str] = &[
    "as","break","const","continue","crate","else","enum","extern",
    "false","fn","for","if","impl","in","let","loop","match","mod",
    "move","mut","pub","ref","return","self","Self","static","struct",
    "super","trait","true","type","unsafe","use","where","while",
    "async","await","dyn","abstract","become","box","do","final",
    "macro","override","priv","try","typeof","unsized","virtual","yield",
];
const KW_ASM: &[&str] = &[
    "section","global","extern","bits","org","align","db","dw","dd",
    "dq","resb","resw","resd","resq","incbin","equ","times","macro",
    "endmacro","struc","endstruc","istruc","at","iend","PROC","ENDP",
    "ASSUME","offset","ptr","byte","word","dword","qword","tbyte",
    "near","far","proc","endp","assume","public","extrn","include",
    "mov","add","sub","mul","div","inc","dec","and","or","xor","not",
    "shl","shr","push","pop","call","ret","jmp","je","jne","jg","jl",
    "jge","jle","cmp","test","int","syscall","lea","nop","hlt",
];

fn keywords(lang: Lang) -> &'static [&'static str] {
    match lang {
        Lang::C      => KW_C,
        Lang::Cpp    => KW_CPP,
        Lang::Csharp => KW_CS,
        Lang::Rust   => KW_RUST,
        Lang::Asm    => KW_ASM,
        Lang::None   => &[],
    }
}

fn is_keyword(word: &str, lang: Lang) -> bool {
    keywords(lang).contains(&word)
}

const COMMENT: Color32 = Color32::from_gray(128);
const STRING:  Color32 = Color32::from_rgb(206, 145, 120);
const KEYWORD: Color32 = Color32::from_rgb(86, 156, 214);
const NUMBER:  Color32 = Color32::from_rgb(181, 206, 168);
const PREPROC: Color32 = Color32::from_rgb(106, 153, 85);
const BRACKET: Color32 = Color32::from_rgb(255, 215, 0);
const TYPE:    Color32 = Color32::from_rgb(78, 201, 176);

fn default_fmt() -> TextFormat {
    TextFormat { font_id: FontId::new(14.0, FontFamily::Monospace), ..Default::default() }
}

fn colored_fmt(color: Color32) -> TextFormat {
    TextFormat { font_id: FontId::new(14.0, FontFamily::Monospace), color, ..Default::default() }
}

pub fn highlight_job(text: &str, lang: Lang) -> LayoutJob {
    let mut sections: Vec<(usize, usize, Color32)> = Vec::new();
    let bytes = text.as_bytes();
    let len = bytes.len();
    let mut i = 0;

    while i < len {
        if i + 1 < len && bytes[i] == b'/' && bytes[i + 1] == b'/' {
            sections.push((i, len, COMMENT));
            break;
        }
        if i + 1 < len && bytes[i] == b'/' && bytes[i + 1] == b'*' {
            let mut end = i + 2;
            while end + 1 < len && !(bytes[end] == b'*' && bytes[end + 1] == b'/') {
                end += 1;
            }
            if end + 1 < len { end += 2; }
            sections.push((i, end, COMMENT));
            i = end;
            continue;
        }
        if bytes[i] == b'"' || bytes[i] == b'\'' {
            let quote = bytes[i];
            let start = i;
            i += 1;
            while i < len && bytes[i] != quote {
                if bytes[i] == b'\\' && i + 1 < len { i += 2; }
                else { i += 1; }
            }
            if i < len { i += 1; }
            sections.push((start, i, STRING));
            continue;
        }
        if bytes[i] == b'#' {
            let start = i;
            while i < len && bytes[i] != b'\n' { i += 1; }
            sections.push((start, i, PREPROC));
            continue;
        }
        if bytes[i].is_ascii_digit() {
            let start = i;
            while i < len && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'.' || bytes[i] == b'x' || bytes[i] == b'X') { i += 1; }
            sections.push((start, i, NUMBER));
            continue;
        }
        if bytes[i].is_ascii_alphabetic() || bytes[i] == b'_' {
            let start = i;
            while i < len && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'_') { i += 1; }
            let word = &text[start..i];
            if is_keyword(word, lang) {
                sections.push((start, i, KEYWORD));
            } else if word.as_bytes()[0].is_ascii_uppercase() {
                sections.push((start, i, TYPE));
            }
            continue;
        }
        if matches!(bytes[i], b'{' | b'}' | b'(' | b')' | b'[' | b']') {
            sections.push((i, i + 1, BRACKET));
            i += 1;
            continue;
        }
        i += 1;
    }

    let mut job = LayoutJob {
        text: text.to_owned(),
        wrap: TextWrapping { max_width: f32::INFINITY, max_rows: usize::MAX, break_anywhere: false, overflow_character: None },
        ..Default::default()
    };

    let mut pos = 0;
    sections.sort_by_key(|s| s.0);
    for (start, end, color) in sections {
        if start > pos {
            job.sections.push(LayoutSection {
                leading_space: 0.0,
                byte_range: pos..start,
                format: default_fmt(),
            });
        }
        job.sections.push(LayoutSection {
            leading_space: 0.0,
            byte_range: start..end,
            format: colored_fmt(color),
        });
        pos = end;
    }
    if pos < len {
        job.sections.push(LayoutSection {
            leading_space: 0.0,
            byte_range: pos..len,
            format: default_fmt(),
        });
    }
    job
}
