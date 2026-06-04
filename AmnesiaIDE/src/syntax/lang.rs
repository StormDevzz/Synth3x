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

pub fn is_keyword(word: &str, lang: Lang) -> bool {
    keywords(lang).contains(&word)
}
