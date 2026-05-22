//! Window state and type enumerations.

/// The state a window can be in.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum WindowState {
    /// State is unknown or unset.
    Unknown,
    /// Normal restored state.
    Restored,
    /// Window is minimized (hidden from view).
    Minimized,
    /// Window is maximized (fills the application zone).
    Maximized,
    /// Window is vertically maximized.
    VertMaximized,
    /// Window is horizontally maximized.
    HorizMaximized,
    /// Window is fullscreen (covers the entire output).
    Fullscreen,
    /// Window is hidden.
    Hidden,
    /// Window is attached to an output edge (e.g., panels).
    Attached,
}

impl WindowState {
    /// Convert from a raw C enum value.
    pub(crate) fn from_raw(value: i32) -> Self {
        match value {
            0 => Self::Unknown,
            1 => Self::Restored,
            2 => Self::Minimized,
            3 => Self::Maximized,
            4 => Self::VertMaximized,
            5 => Self::Fullscreen,
            6 => Self::HorizMaximized,
            7 => Self::Hidden,
            8 => Self::Attached,
            _ => Self::Unknown,
        }
    }

    /// Convert to a raw C enum value.
    pub(crate) fn to_raw(self) -> i32 {
        match self {
            Self::Unknown => 0,
            Self::Restored => 1,
            Self::Minimized => 2,
            Self::Maximized => 3,
            Self::VertMaximized => 4,
            Self::Fullscreen => 5,
            Self::HorizMaximized => 6,
            Self::Hidden => 7,
            Self::Attached => 8,
        }
    }
}

impl Default for WindowState {
    fn default() -> Self {
        Self::Unknown
    }
}

/// The type of a window, describing its role.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum WindowType {
    /// A regular application window.
    Normal,
    /// A utility/floating window.
    Utility,
    /// A dialog window.
    Dialog,
    /// A gloss window.
    Gloss,
    /// A freestyle window (no constraints on shape/decoration).
    Freestyle,
    /// A menu window.
    Menu,
    /// An input method window (e.g., on-screen keyboard).
    InputMethod,
    /// A satellite/toolbar window.
    Satellite,
    /// A tooltip window.
    Tip,
    /// A decoration window.
    Decoration,
}

impl WindowType {
    /// Convert from a raw C enum value.
    pub(crate) fn from_raw(value: i32) -> Self {
        match value {
            0 => Self::Normal,
            1 => Self::Utility,
            2 => Self::Dialog,
            3 => Self::Gloss,
            4 => Self::Freestyle,
            5 => Self::Menu,
            6 => Self::InputMethod,
            7 => Self::Satellite,
            8 => Self::Tip,
            9 => Self::Decoration,
            _ => Self::Normal,
        }
    }

    /// Convert to a raw C enum value.
    #[allow(dead_code)]
    pub(crate) fn to_raw(self) -> i32 {
        match self {
            Self::Normal => 0,
            Self::Utility => 1,
            Self::Dialog => 2,
            Self::Gloss => 3,
            Self::Freestyle => 4,
            Self::Menu => 5,
            Self::InputMethod => 6,
            Self::Satellite => 7,
            Self::Tip => 8,
            Self::Decoration => 9,
        }
    }
}

impl Default for WindowType {
    fn default() -> Self {
        Self::Normal
    }
}
