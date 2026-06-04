use std::process::{Command, Child, ExitStatus, Stdio};
use std::time::Duration;
use std::io::Result;

/// Exit reason for a supervised process.
#[derive(Debug, Clone, PartialEq)]
pub enum ExitReason {
    Success,
    Crash(i32),
    Signal(i32),
    Unknown,
}

/// Describes what to do after a process exits.
#[derive(Debug, Clone, PartialEq)]
pub enum RestartPolicy {
    Always,
    OnFailure,
    Never,
}

/// A supervised child process with auto-restart capability.
pub struct SafeProcess {
    name: String,
    command: String,
    args: Vec<String>,
    child: Option<Child>,
    restart_policy: RestartPolicy,
    max_restarts: u32,
    restart_count: u32,
    backoff: Duration,
}

impl SafeProcess {
    pub fn new<S: Into<String>>(name: S, command: S) -> Self {
        Self {
            name: name.into(),
            command: command.into(),
            args: Vec::new(),
            child: None,
            restart_policy: RestartPolicy::OnFailure,
            max_restarts: 5,
            restart_count: 0,
            backoff: Duration::from_millis(500),
        }
    }

    pub fn arg<S: Into<String>>(&mut self, arg: S) -> &mut Self {
        self.args.push(arg.into());
        self
    }

    pub fn args(&mut self, args: &[String]) -> &mut Self {
        self.args.extend_from_slice(args);
        self
    }

    pub fn restart_policy(&mut self, policy: RestartPolicy) -> &mut Self {
        self.restart_policy = policy;
        self
    }

    pub fn max_restarts(&mut self, n: u32) -> &mut Self {
        self.max_restarts = n;
        self
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn restart_count(&self) -> u32 {
        self.restart_count
    }

    /// Spawn the process.
    pub fn start(&mut self) -> Result<()> {
        let mut cmd = Command::new(&self.command);
        cmd.args(&self.args)
            .stdin(Stdio::null())
            .stdout(Stdio::null())
            .stderr(Stdio::null());
        self.child = Some(cmd.spawn()?);
        Ok(())
    }

    /// Send SIGTERM to the process.
    pub fn stop(&mut self) -> Result<()> {
        if let Some(ref mut child) = self.child {
            let _ = child.kill();
            let _ = child.wait();
        }
        self.child = None;
        Ok(())
    }

    /// Force kill (SIGKILL).
    pub fn kill(&mut self) -> Result<()> {
        if let Some(ref mut child) = self.child {
            let _ = child.kill();
            let _ = child.wait();
        }
        self.child = None;
        Ok(())
    }

    /// Check if the process is still running.
    pub fn is_running(&mut self) -> bool {
        if let Some(ref mut child) = self.child {
            match child.try_wait() {
                Ok(None) => true,
                _ => false,
            }
        } else {
            false
        }
    }

    /// Wait for the process to exit and return the reason.
    pub fn wait(&mut self) -> Result<ExitReason> {
        match self.child.as_mut() {
            Some(child) => {
                let status = child.wait()?;
                self.child = None;
                Ok(exit_reason(status))
            }
            None => Ok(ExitReason::Unknown),
        }
    }

    /// Try to restart the process if allowed by policy.
    /// Returns true if restarted, false if policy says stop.
    pub fn try_restart(&mut self) -> bool {
        if self.restart_count >= self.max_restarts {
            return false;
        }

        match self.restart_policy {
            RestartPolicy::Never => false,
            RestartPolicy::Always => {
                std::thread::sleep(self.backoff);
                self.backoff = (self.backoff * 2).min(Duration::from_secs(30));
                self.restart_count += 1;
                self.start().is_ok()
            }
            RestartPolicy::OnFailure => {
                // Check exit status
                if let Some(ref mut child) = self.child {
                    if let Ok(Some(status)) = child.try_wait() {
                        if status.success() {
                            return false;
                        }
                    }
                }
                std::thread::sleep(self.backoff);
                self.backoff = (self.backoff * 2).min(Duration::from_secs(30));
                self.restart_count += 1;
                self.start().is_ok()
            }
        }
    }

    /// Reset restart count after a stable period.
    pub fn reset_restart_count(&mut self) {
        self.restart_count = 0;
        self.backoff = Duration::from_millis(500);
    }
}

fn exit_reason(status: ExitStatus) -> ExitReason {
    use std::os::unix::process::ExitStatusExt;
    if status.success() {
        ExitReason::Success
    } else if let Some(code) = status.code() {
        ExitReason::Crash(code)
    } else if let Some(signal) = status.signal() {
        ExitReason::Signal(signal)
    } else {
        ExitReason::Unknown
    }
}

/// Supervisor manages multiple SafeProcess instances.
pub struct Supervisor {
    processes: Vec<SafeProcess>,
}

impl Supervisor {
    pub fn new() -> Self {
        Self { processes: Vec::new() }
    }

    pub fn add(&mut self, proc: SafeProcess) {
        self.processes.push(proc);
    }

    pub fn start_all(&mut self) -> Result<()> {
        for proc in &mut self.processes {
            proc.start()?;
        }
        Ok(())
    }

    pub fn stop_all(&mut self) -> Result<()> {
        for proc in &mut self.processes {
            let _ = proc.stop();
        }
        Ok(())
    }

    /// Poll all processes; restart any that have died according to policy.
    pub fn poll(&mut self) {
        for proc in &mut self.processes {
            if !proc.is_running() {
                proc.try_restart();
            }
        }
    }

    /// Run the event loop: poll every `interval`.
    pub fn run(&mut self, interval: Duration) {
        loop {
            self.poll();
            std::thread::sleep(interval);
        }
    }

    pub fn iter(&self) -> std::slice::Iter<'_, SafeProcess> {
        self.processes.iter()
    }

    pub fn iter_mut(&mut self) -> std::slice::IterMut<'_, SafeProcess> {
        self.processes.iter_mut()
    }
}
