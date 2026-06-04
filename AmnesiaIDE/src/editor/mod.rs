pub mod update;
mod actions;
pub mod views;

use crate::file::tree::FEntry;
use crate::file::Workspace;
use crate::syntax::Lang;
use crate::notifications::store::Notify;
use crate::icon::loader::IconSet;


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
    pub home: String,
    pub notify: Notify,

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

    // context menu for file tree
    pub ctx_file: Option<FEntry>,
    pub rename_target: Option<String>,
    pub rename_buf: String,

    // project creation dialog
    pub new_project: crate::dialog::NewProjectState,

    // github
    pub github: crate::github::GitHubState,

    // icons
    pub icons: IconSet,
}

impl Default for AmnesiaApp {
    fn default() -> Self {
        let home = crate::creations::dirs::ensure_home();
        let mut n = Notify::new();
        n.push(crate::notifications::store::Level::Info, format!("Home: {}", home));
        let _installed = crate::install::check::check_all(&mut n);

        Self {
            screen: Screen::Welcome,
            ws: None,
            msg: String::new(),
            home,
            notify: n,
            text: String::new(),
            file_path: String::new(),
            dirty: false,
            lang: Lang::None,
            cursor_line: 0, cursor_col: 0,
            show_term: false, show_files: true,
            dialog: Dialog::None,
            dialog_path: String::new(),
            ctx_file: None,
            rename_target: None,
            rename_buf: String::new(),
            new_project: crate::dialog::NewProjectState::default(),
            github: crate::github::GitHubState::default(),
            icons: IconSet::empty(),
        }
    }
}

impl AmnesiaApp {
    pub fn init_icons(&mut self, ctx: &egui::Context) {
        self.icons = crate::icon::loader::load_all(ctx);
    }
}
