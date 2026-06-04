use std::fmt;
use std::ops::Drop;

/// A string that zeroes its memory on drop.
/// Used for passwords, keys, and other sensitive data.
pub struct SecureString {
    inner: Vec<u8>,
    revealed: bool,
}

impl SecureString {
    pub fn new() -> Self {
        Self { inner: Vec::new(), revealed: false }
    }

    pub fn from(s: &str) -> Self {
        Self { inner: s.as_bytes().to_vec(), revealed: false }
    }

    /// Reveal the string as a `&str`.
    /// Use sparingly; the string remains in memory until drop.
    pub fn reveal(&mut self) -> &str {
        self.revealed = true;
        std::str::from_utf8(&self.inner).unwrap_or("")
    }

    pub fn is_empty(&self) -> bool {
        self.inner.is_empty()
    }

    pub fn len(&self) -> usize {
        self.inner.len()
    }
}

impl Drop for SecureString {
    fn drop(&mut self) {
        for byte in &mut self.inner {
            *byte = 0;
        }
        self.inner.clear();
    }
}

impl fmt::Debug for SecureString {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("SecureString(***)")
    }
}

/// A simple counter that can be used for rate-limiting
/// or tracking restart attempts.
pub struct MonotonicCounter {
    val: u64,
}

impl MonotonicCounter {
    pub const fn new() -> Self {
        Self { val: 0 }
    }

    pub fn tick(&mut self) -> u64 {
        self.val += 1;
        self.val
    }

    pub fn value(&self) -> u64 {
        self.val
    }
}
