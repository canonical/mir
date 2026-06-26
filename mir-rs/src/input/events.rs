//! Input event struct types.

use super::modifiers::Modifiers;
use super::{KeyAction, PointerAction, TouchAction};

/// A generic input event (base type for all input).
///
/// Used in methods like `handle_request_move` where the specific event
/// type doesn't matter.
#[derive(Debug, Clone)]
pub struct InputEvent {
    /// The raw event timestamp in nanoseconds.
    pub timestamp_ns: i64,
}

/// A keyboard event.
///
/// Provided to [`WindowManagementPolicy::handle_keyboard_event`](crate::policy::WindowManagementPolicy::handle_keyboard_event).
#[derive(Debug, Clone)]
pub struct KeyboardEvent {
    /// The action (press, release, repeat).
    pub action: KeyAction,
    /// The Linux key code (from `linux/input-event-codes.h`).
    pub key_code: i32,
    /// The XKB keysym.
    pub keysym: u32,
    /// Active modifier flags.
    pub modifiers: Modifiers,
    /// Timestamp in nanoseconds.
    pub timestamp_ns: i64,
}

/// A pointer (mouse) event.
///
/// Provided to [`WindowManagementPolicy::handle_pointer_event`](crate::policy::WindowManagementPolicy::handle_pointer_event).
#[derive(Debug, Clone)]
pub struct PointerEvent {
    /// The pointer action.
    pub action: PointerAction,
    /// The absolute X position.
    pub x: f32,
    /// The absolute Y position.
    pub y: f32,
    /// The horizontal scroll amount.
    pub hscroll: f32,
    /// The vertical scroll amount.
    pub vscroll: f32,
    /// Active modifier flags.
    pub modifiers: Modifiers,
    /// The button that triggered this event (for button actions).
    pub button: u32,
    /// Timestamp in nanoseconds.
    pub timestamp_ns: i64,
}

/// A touch event.
///
/// Provided to [`WindowManagementPolicy::handle_touch_event`](crate::policy::WindowManagementPolicy::handle_touch_event).
#[derive(Debug, Clone)]
pub struct TouchEvent {
    /// The touch action.
    pub action: TouchAction,
    /// The touch point ID (for multi-touch).
    pub touch_id: i32,
    /// The X position of the touch point.
    pub x: f32,
    /// The Y position of the touch point.
    pub y: f32,
    /// Active modifier flags.
    pub modifiers: Modifiers,
    /// Timestamp in nanoseconds.
    pub timestamp_ns: i64,
}
