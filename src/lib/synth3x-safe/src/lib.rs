pub mod process;
pub mod privsep;
pub mod hwdetect;
pub mod secure;

pub const VERSION: &str = env!("SYNTH3X_VERSION");
pub const VERSION_TAG: &str = env!("SYNTH3X_VERSION_TAG");
pub const OS_NAME: &str = "Synth3x";
