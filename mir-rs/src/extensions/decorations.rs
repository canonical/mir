//! Window decoration configuration.
//!
//! Controls whether the compositor provides server-side decorations (SSD)
//! or the client draws its own (CSD).

use super::ServerExtension;

use std::pin::Pin;

/// Controls the window decoration strategy.
///
/// Compositors can prefer client-side decorations (CSD), server-side
/// decorations (SSD), or allow the client to choose.
#[derive(Debug, Clone)]
pub struct Decorations {
    strategy: Strategy,
}

/// The decoration strategy to use.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Strategy {
    /// Prefer client-side decorations.
    PreferCsd,
    /// Prefer server-side decorations.
    PreferSsd,
    /// Always use server-side decorations.
    AlwaysSsd,
    /// Always use client-side decorations.
    AlwaysCsd,
}

impl Decorations {
    /// Prefer client-side decorations (CSD).
    ///
    /// Clients that support CSD will draw their own title bars and borders.
    /// Clients that don't will get server-side decorations.
    pub fn prefer_csd() -> Self {
        Self {
            strategy: Strategy::PreferCsd,
        }
    }

    /// Prefer server-side decorations (SSD).
    ///
    /// The compositor will draw title bars and borders for all windows
    /// unless the client explicitly requests otherwise.
    pub fn prefer_ssd() -> Self {
        Self {
            strategy: Strategy::PreferSsd,
        }
    }

    /// Always use server-side decorations.
    ///
    /// The compositor will always draw title bars and borders, regardless
    /// of client preferences.
    pub fn always_ssd() -> Self {
        Self {
            strategy: Strategy::AlwaysSsd,
        }
    }

    /// Always use client-side decorations.
    ///
    /// Clients will always be responsible for drawing their own title bars
    /// and borders, with no server-side fallback.
    pub fn always_csd() -> Self {
        Self {
            strategy: Strategy::AlwaysCsd,
        }
    }
}

impl ServerExtension for Decorations {
    fn name(&self) -> &str {
        match self.strategy {
            Strategy::PreferCsd => "Decorations(PreferCSD)",
            Strategy::PreferSsd => "Decorations(PreferSSD)",
            Strategy::AlwaysSsd => "Decorations(AlwaysSSD)",
            Strategy::AlwaysCsd => "Decorations(AlwaysCSD)",
        }
    }

    fn apply(self: Box<Self>, runner: Pin<&mut mir_sys::ffi::MiralRunner>) {
        let mode = match self.strategy {
            Strategy::PreferCsd => 1,
            Strategy::PreferSsd => 2,
            Strategy::AlwaysSsd => 3,
            Strategy::AlwaysCsd => 4,
        };
        mir_sys::ffi::miral_runner_add_decorations(runner, mode);
    }
}
