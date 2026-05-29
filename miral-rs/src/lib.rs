/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//! Idiomatic Rust API for building Wayland compositors with the Mir display server.
//!
//! This crate provides safe, ergonomic abstractions over the battle-tested miral C++ library.
//! Compositor authors implement the [`policy::WindowManagementPolicy`] trait to define custom
//! window management behavior, then use [`runner::MirRunner`] to start the compositor.
//!
//! # Quick Start
//!
//! ```rust,no_run
//! use miral::prelude::*;
//!
//! #[derive(Default)]
//! struct MyPolicy {
//!     tools: WindowManagerTools,
//! }
//!
//! impl WindowManagementPolicy for MyPolicy {
//!     fn tools(&self) -> &WindowManagerTools { &self.tools }
//!     fn tools_mut(&mut self) -> &mut WindowManagerTools { &mut self.tools }
//! }
//!
//! fn main() {
//!     MirRunner::new(std::env::args())
//!         .add(Decorations::prefer_csd())
//!         .add_window_management_policy::<MyPolicy>()
//!         .run()
//!         .expect("Server failed");
//! }
//! ```

#![deny(missing_docs)]

pub mod application;
pub mod client;
pub mod configuration;
pub mod decorations;
pub mod extensions;
pub mod geometry;
pub mod idle;
pub mod input;
pub mod keymap;
pub mod magnifier;
pub mod output;
pub mod policy;
pub mod runner;
pub mod session_lock;
pub mod window;
pub mod workspace;

/// Convenience re-exports for the most commonly used types.
///
/// Import with `use miral::prelude::*` to get everything needed for a basic compositor.
pub mod prelude {
    pub use crate::application::{Application, ApplicationInfo};
    pub use crate::client::ExternalClientLauncher;
    pub use crate::configuration::ConfigurationOption;
    pub use crate::decorations::Decorations;
    pub use crate::extensions::{ServerExtension, WaylandExtensions, X11Support};
    pub use crate::geometry::{Displacement, Point, PointF, Rectangle, Size};
    pub use crate::idle::IdleListener;
    pub use crate::input::{InputEvent, KeyAction, KeyboardEvent, PointerEvent, TouchEvent};
    pub use crate::keymap::Keymap;
    pub use crate::magnifier::Magnifier;
    pub use crate::output::{
        FormFactor, Orientation, Output, OutputType, PhysicalSizeMM, PowerMode, Zone,
    };
    pub use crate::policy::{Advice, WindowManagementPolicy, WindowManagerTools};
    pub use crate::runner::{MirRunner, RunnerHandle};
    pub use crate::session_lock::SessionLockListener;
    pub use crate::window::{
        AspectRatio, DepthLayer, FocusMode, FocusStealing, InputReceptionMode, OrientationMode,
        PlacementGravity, PlacementHints, PointerConfinementState, ResizeEdge, ShellChrome,
        TiledEdges, Window, WindowInfo, WindowSpecification, WindowState, WindowType,
    };
}
