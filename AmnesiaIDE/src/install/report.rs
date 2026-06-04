use crate::install::compilers::Compiler;
use crate::install::status::Status;

pub fn format(results: &[(&Compiler, Status)]) -> String {
    let mut s = String::new();
    for (c, st) in results {
        s.push_str(&format!("{} {}  {}\n", crate::icon::status::for_status(st), c.name, st.detail()));
    }
    s
}

pub fn all_ok(results: &[(&Compiler, Status)]) -> bool {
    results.iter().all(|(_, s)| matches!(s, Status::Found { .. }))
}

pub fn missing<'a>(results: &'a [(&'a Compiler, Status)]) -> Vec<&'a Compiler> {
    results.iter().filter_map(|(c, s)| match s {
        Status::NotFound => Some(*c),
        _ => None,
    }).collect()
}
