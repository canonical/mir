//! Keyboard modifier flags.

bitflags::bitflags! {
    /// Keyboard modifier flags.
    ///
    /// Represents which modifier keys are active during an input event.
    /// Bit values match `MirInputEventModifier` from
    /// `include/core/mir_toolkit/events/enums.h`.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub struct Modifiers: u32 {
        /// No modifier (Mir uses bit 0 for "none").
        const NONE        = 1 << 0;
        /// Alt (either side).
        const ALT         = 1 << 1;
        /// Left Alt.
        const ALT_LEFT    = 1 << 2;
        /// Right Alt.
        const ALT_RIGHT   = 1 << 3;
        /// Shift (either side).
        const SHIFT       = 1 << 4;
        /// Left Shift.
        const SHIFT_LEFT  = 1 << 5;
        /// Right Shift.
        const SHIFT_RIGHT = 1 << 6;
        /// Sym.
        const SYM         = 1 << 7;
        /// Function.
        const FUNCTION    = 1 << 8;
        /// Control (either side).
        const CTRL        = 1 << 9;
        /// Left Control.
        const CTRL_LEFT   = 1 << 10;
        /// Right Control.
        const CTRL_RIGHT  = 1 << 11;
        /// Meta/Super (either side).
        const META        = 1 << 12;
        /// Left Meta/Super.
        const META_LEFT   = 1 << 13;
        /// Right Meta/Super.
        const META_RIGHT  = 1 << 14;
        /// Caps Lock.
        const CAPS_LOCK   = 1 << 15;
        /// Num Lock.
        const NUM_LOCK    = 1 << 16;
        /// Scroll Lock.
        const SCROLL_LOCK = 1 << 17;
    }
}

impl Modifiers {
    /// Create modifiers from raw bitfield.
    pub(crate) fn from_raw(raw: u32) -> Self {
        Self::from_bits_truncate(raw)
    }

    /// Whether Alt is held.
    pub fn alt(&self) -> bool {
        self.intersects(Self::ALT)
    }

    /// Whether Shift is held.
    pub fn shift(&self) -> bool {
        self.intersects(Self::SHIFT)
    }

    /// Whether Control is held.
    pub fn ctrl(&self) -> bool {
        self.intersects(Self::CTRL)
    }

    /// Whether Super/Meta is held.
    pub fn meta(&self) -> bool {
        self.intersects(Self::META)
    }
}
