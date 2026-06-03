//! Input action enums for keyboard, pointer, and touch.

/// Keyboard key actions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum KeyAction {
    /// A key was pressed down.
    Down,
    /// A key was released.
    Up,
    /// A key is being held (repeat).
    Repeat,
    /// A modifier-only state change (no physical key press/release).
    Modifiers,
}

impl KeyAction {
    /// Convert from raw C enum value (`MirKeyboardAction`).
    ///
    /// Returns `None` for unrecognised values.
    pub(crate) fn from_raw(value: i32) -> Option<Self> {
        match value {
            0 => Some(Self::Up),
            1 => Some(Self::Down),
            2 => Some(Self::Repeat),
            3 => Some(Self::Modifiers),
            _ => None,
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
            2 => Self::Enter,
            3 => Self::Leave,
            4 => Self::Motion,
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
