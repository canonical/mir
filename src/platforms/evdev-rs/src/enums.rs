
#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirEventType {
    Key,
    Motion,
    Window,
    Resize,
    PromptSessionStateChange,
    Orientation,
    CloseWindow,
    Input,
    InputConfiguration,
    WindowOutput,
    InputDeviceState,
    WindowPlacement,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirInputEventType {
    Key = 0,
    Touch = 1,
    Pointer = 2,
    KeyboardResync = 3,
    Types,
}

#[repr(u32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirInputEventModifier {
    None         = 1 << 0,
    Alt          = 1 << 1,
    AltLeft      = 1 << 2,
    AltRight     = 1 << 3,
    Shift        = 1 << 4,
    ShiftLeft    = 1 << 5,
    ShiftRight   = 1 << 6,
    Sym          = 1 << 7,
    Function     = 1 << 8,
    Ctrl         = 1 << 9,
    CtrlLeft     = 1 << 10,
    CtrlRight    = 1 << 11,
    Meta         = 1 << 12,
    MetaLeft     = 1 << 13,
    MetaRight    = 1 << 14,
    CapsLock     = 1 << 15,
    NumLock      = 1 << 16,
    ScrollLock   = 1 << 17,
}

pub type MirInputEventModifiers = u32;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirKeyboardAction {
    Up,
    Down,
    Repeat,
    Modifiers,
    Actions,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirTouchAction {
    Up = 0,
    Down = 1,
    Change = 2,
    Actions,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirTouchAxis {
    X = 0,
    Y = 1,
    Pressure = 2,
    TouchMajor = 3,
    TouchMinor = 4,
    Size = 5,
    Axes,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirTouchTooltype {
    Unknown = 0,
    Finger = 1,
    Stylus = 2,
    Tooltypes,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirPointerAction {
    ButtonUp = 0,
    ButtonDown = 1,
    Enter = 2,
    Leave = 3,
    Motion = 4,
    Actions,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirPointerAxis {
    X = 0,
    Y = 1,
    VScroll = 2,
    HScroll = 3,
    RelativeX = 4,
    RelativeY = 5,
    VScrollDiscrete = 6,
    HScrollDiscrete = 7,
    VScrollValue120 = 8,
    HScrollValue120 = 9,
    Axes,
}

#[repr(u32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirPointerButton {
    Primary   = 1 << 0,
    Secondary = 1 << 1,
    Tertiary  = 1 << 2,
    Back      = 1 << 3,
    Forward   = 1 << 4,
    Side      = 1 << 5,
    Extra     = 1 << 6,
    Task      = 1 << 7,
}

pub type MirPointerButtons = u32;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MirPointerAxisSource {
    None,
    Wheel,
    Finger,
    Continuous,
    WheelTilt,
}
