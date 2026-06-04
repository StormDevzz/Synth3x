#[derive(Clone)]
pub enum Status {
    NotFound,
    Found { version: String },
    Error(String),
}

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
