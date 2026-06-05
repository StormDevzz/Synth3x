#[derive(Clone)]
pub enum Status {
    NotFound,
    Found { #[allow(dead_code)] version: String },
    Error(String),
}

#[allow(dead_code)]
impl Status {
    pub fn label(&self) -> &str {
        match self {
            Status::NotFound => "not found",
            Status::Found { .. } => "OK",
            Status::Error(_) => "error",
        }
    }

    pub fn detail(&self) -> String {
        match self {
            Status::NotFound => "not installed".into(),
            Status::Found { version } => version.clone(),
            Status::Error(e) => e.clone(),
        }
    }
}
