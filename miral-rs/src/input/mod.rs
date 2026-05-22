//! Input event types for keyboard, pointer, and touch.
//!
//! These types are provided to policy methods for handling user input.

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

/// Keyboard key actions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum KeyAction {
    /// A key was pressed down.
    Down,
    /// A key was released.
    Up,
    /// A key is being held (repeat).
    Repeat,
}

impl KeyAction {
    /// Convert from raw C enum value.
    pub(crate) fn from_raw(value: i32) -> Self {
        match value {
            0 => Self::Up,
            1 => Self::Down,
            2 => Self::Repeat,
            _ => Self::Down,
        }
    }
}

/// Pointer (mouse) actions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PointerAction {
    /// The pointer was moved.
    Motion,
    /// A button was pressed.
    ButtonDown,
    /// A button was released.
    ButtonUp,
    /// Enter the surface.
    Enter,
    /// Leave the surface.
    Leave,
}

impl PointerAction {
    /// Convert from raw C enum value.
    pub(crate) fn from_raw(value: i32) -> Self {
        match value {
            0 => Self::ButtonUp,
            1 => Self::ButtonDown,
            2 => Self::Motion,
            3 => Self::Enter,
            4 => Self::Leave,
            _ => Self::Motion,
        }
    }
}

/// Touch actions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TouchAction {
    /// A touch point was placed on the surface.
    Down,
    /// A touch point was lifted.
    Up,
    /// A touch point moved.
    Change,
}

impl TouchAction {
    /// Convert from raw C enum value.
    pub(crate) fn from_raw(value: i32) -> Self {
        match value {
            0 => Self::Up,
            1 => Self::Down,
            2 => Self::Change,
            _ => Self::Change,
        }
    }
}

/// Keyboard modifier flags.
///
/// Represents which modifier keys are active during an input event.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Modifiers {
    raw: u32,
}

impl Modifiers {
    /// Create modifiers from raw bitfield.
    pub(crate) fn from_raw(raw: u32) -> Self {
        Self { raw }
    }

    /// Whether no modifiers are active.
    pub fn is_empty(&self) -> bool {
        self.raw == 0
    }

    /// Whether Alt is held.
    pub fn alt(&self) -> bool {
        self.raw & (1 << 2) != 0
    }

    /// Whether Shift is held.
    pub fn shift(&self) -> bool {
        self.raw & (1 << 0) != 0
    }

    /// Whether Control is held.
    pub fn ctrl(&self) -> bool {
        self.raw & (1 << 1) != 0
    }

    /// Whether Super/Meta is held.
    pub fn meta(&self) -> bool {
        self.raw & (1 << 3) != 0
    }
}
