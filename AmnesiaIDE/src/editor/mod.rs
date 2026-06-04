pub mod update;
mod actions;
pub mod views;

use crate::file::Workspace;
use crate::syntax::Lang;

#[derive(Default, PartialEq)]
pub enum Dialog {
    #[default] None,
    Open, SaveAs,
    NewWorkspace, OpenWorkspace,
    NewFile,
}

pub enum Screen {
    Welcome,
    Workspace,
}

pub struct AmnesiaApp {
    pub screen: Screen,
    pub ws: Option<Workspace>,
    pub msg: String,

    // editor
    pub text: String,
    pub file_path: String,
    pub dirty: bool,
    pub lang: Lang,
    pub cursor_line: usize,
    pub cursor_col: usize,

    // ui toggles
    pub show_term: bool,
    pub show_files: bool,

    pub dialog: Dialog,
    pub dialog_path: String,
}

impl Default for AmnesiaApp {
    fn default() -> Self {
        Self {
            screen: Screen::Welcome,
            ws: None,
            msg: String::new(),
            text: String::new(),
            file_path: String::new(),
            dirty: false,
            lang: Lang::None,
            cursor_line: 0, cursor_col: 0,
            show_term: false, show_files: true,
            dialog: Dialog::None,
            dialog_path: String::new(),
        }
    }
}
