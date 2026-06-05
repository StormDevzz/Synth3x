use crate::file::tree::{FEntry, scan_dir};
use crate::file::ops;
use crate::terminal::Term;

pub struct Workspace {
    pub path: String,
    pub files: Vec<FEntry>,
    pub term: Term,
}

impl Workspace {
    pub fn open(path: &str) -> Result<Self, String> {
        if !std::fs::metadata(path).map(|m| m.is_dir()).unwrap_or(false) {
            return Err(format!("Not a directory: {}", path));
        }
        let files = scan_dir(path, 0);
        let term = Term::spawn(path);
        Ok(Workspace { path: path.to_owned(), files, term })
    }

    #[allow(dead_code)]
    pub fn create_and_open(path: &str) -> Result<Self, String> {
        ops::create_dir(path)?;
        Self::open(path)
    }

    pub fn refresh(&mut self) {
        self.files = scan_dir(&self.path, 0);
    }
}
