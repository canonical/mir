//! Input event types for keyboard, pointer, and touch.
//!
//! These types are provided to policy methods for handling user input.

mod actions;
mod events;
pub(crate) mod modifiers;

pub use actions::{KeyAction, PointerAction, TouchAction};
pub use events::{InputEvent, KeyboardEvent, PointerEvent, TouchEvent};
pub use modifiers::Modifiers;
