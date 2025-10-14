mod enums;
use crate::enums::{
    MirDeviceCapability, MirKeyboardAction, MirPointerAcceleration, MirPointerAction,
    MirPointerAxisSource, MirPointerButton, MirPointerHandedness,
};
use cxx::{SharedPtr, UniquePtr};
use input::event::keyboard::{KeyState, KeyboardEventTrait};
use input::event::pointer::{ButtonState, PointerEventTrait, PointerScrollEvent};
use input::event::{DeviceEvent, EventTrait, KeyboardEvent, PointerEvent};
use input::{AsRaw, Device, DeviceCapability, Event, Libinput, LibinputInterface};
use libc::{major, minor, poll, pollfd, stat, POLLIN};
use nix::unistd::close;
use std::ffi::CString;
use std::os::fd::{AsFd, AsRawFd, BorrowedFd, FromRawFd};
use std::os::unix::ffi::OsStrExt;
use std::os::unix::io::OwnedFd;
use std::path::Path;
use std::sync::mpsc;
use std::thread::{self, JoinHandle};
use std::{pin::Pin, ptr::NonNull};

struct LibinputInterfaceImpl {
    bridge: SharedPtr<PlatformBridgeC>,
    fds: Vec<UniquePtr<DeviceBridgeC>>,
}

impl LibinputInterface for LibinputInterfaceImpl {
    // This method is called whenever libinput needs to open a new device.
    fn open_restricted(&mut self, path: &Path, flags: i32) -> Result<OwnedFd, i32> {
        println!("Opening device: {:?} with flags: {}", path, flags);
        let cpath = CString::new(path.as_os_str().as_bytes()).unwrap();

        let mut st: stat = unsafe { std::mem::zeroed() };
        let ret = unsafe { libc::stat(cpath.as_ptr(), &mut st) };
        if ret != 0 {
            return Err(ret);
        }

        let major_num = major(st.st_rdev);
        let minor_num = minor(st.st_rdev);

        // By keeping the device referenced in the fds vector, we ensure it stays alive
        // until close_restricted is called.
        let device = self
            .bridge
            .acquire_device(major_num as i32, minor_num as i32);
        let fd = device.raw_fd();
        println!(
            "Acquired fd: {} for device with major: {}, minor: {}",
            fd, major_num, minor_num
        );
        self.fds.push(device);

        let owned = unsafe { OwnedFd::from_raw_fd(fd) };
        println!("Owned fd: {}", owned.as_raw_fd().to_string());
        Ok(owned)
    }

    // This method is called when libinput is done with a device.
    fn close_restricted(&mut self, fd: OwnedFd) {
        let raw_fd = fd.as_raw_fd();
        println!("Closing device with fd: {}", raw_fd.to_string());
        let _ = close(fd);
        // self.fds.retain(|f| f.as_raw_fd() != raw_fd);
    }
}

// This is a bit of a hack here.
//
// The other side of our platform bridge is known by us to be thread-safe, so we can
// safely send it to another thread. However, Rust has no way of knowing that, so
// we have to assert it ourselves.
unsafe impl Send for PlatformBridgeC {}
unsafe impl Sync for PlatformBridgeC {}
unsafe impl Send for InputDeviceRegistry {}
unsafe impl Sync for InputDeviceRegistry {}
unsafe impl Send for InputDevice {}
unsafe impl Sync for InputDevice {}
unsafe impl Send for InputSink {}
unsafe impl Sync for InputSink {}
unsafe impl Send for EventBuilder {}
unsafe impl Sync for EventBuilder {}

// This is another hack.
//
// Because *mut InputSink and *mut EventBuilder are raw pointers, Rust assumes
// that they are neither Send nor Sync. However, we know that the other side of
// the pointer is actually a C++ object that is thread-safe, so we can assert
// that these pointers are Send and Sync. We cannot define Send and Sync on the
// raw types to fix the issue unfortuately, so we have to wrap them in a newtype
// and define Send and Sync on that.
pub struct InputSinkPtr(NonNull<InputSink>);
impl InputSinkPtr {
    fn handle_input(&mut self, event: &SharedPtr<ffi::MirEvent>) {
        unsafe {
            // Pin the raw pointer
            let pinned = Pin::new_unchecked(self.0.as_mut());
            pinned.handle_input(event);
        }
    }
}

unsafe impl Send for InputSinkPtr {}
unsafe impl Sync for InputSinkPtr {}

pub struct EventBuilderPtr(NonNull<EventBuilder>);
unsafe impl Send for EventBuilderPtr {}
unsafe impl Sync for EventBuilderPtr {}

struct DeviceWrapper {
    id: i32,
    device: Device,
    input_device: SharedPtr<InputDevice>,
    input_sink: Option<InputSinkPtr>,
    event_builder: Option<UniquePtr<EventBuilderWrapper>>,
}

struct LibinputState {
    libinput: Libinput,
    known_devices: Vec<DeviceWrapper>,
    next_device_id: i32,
    button_state: u32,
    pointer_x: f32,
    pointer_y: f32,
    scroll_axis_x_accum: f64,
    scroll_axis_y_accum: f64,
    x_scroll_scale: f64,
    y_scroll_scale: f64,
}

pub struct PlatformRs {
    bridge: SharedPtr<PlatformBridgeC>,
    device_registry: SharedPtr<InputDeviceRegistry>,
    handle: Option<JoinHandle<()>>,
    wfd: Option<OwnedFd>,
    tx: Option<mpsc::Sender<ThreadCommand>>,
}

pub struct InputDeviceInfoRs {
    name: String,
    unique_id: String,
    capabilities: u32,
}

impl InputDeviceInfoRs {
    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn unique_id(&self) -> &str {
        &self.unique_id
    }

    pub fn capabilities(&self) -> u32 {
        self.capabilities
    }
}

pub enum ThreadCommand {
    Start(i32, InputSinkPtr, EventBuilderPtr),
    GetDeviceInfo(i32, mpsc::Sender<InputDeviceInfoRs>),
    GetPointerSettings(i32, mpsc::Sender<ffi::PointerSettingsRs>),
    SetPointerSettings(i32, ffi::PointerSettingsC),
}

// This is so silly.
//
// We can't create the channel inside of the PlatformRs implementation because
// the "mod ffi" block gets confused when parsing the template, so it thinks that the
// closing bracket is an equality operator. Moving it out to the global scope fixes
// things, but this is so silly.
fn create_thread_command_channel() -> (mpsc::Sender<ThreadCommand>, mpsc::Receiver<ThreadCommand>) {
    mpsc::channel()
}

fn create_device_info_rs_channel() -> (
    mpsc::Sender<InputDeviceInfoRs>,
    mpsc::Receiver<InputDeviceInfoRs>,
) {
    mpsc::channel()
}

fn create_pointer_settings_rs_channel() -> (
    mpsc::Sender<ffi::PointerSettingsRs>,
    mpsc::Receiver<ffi::PointerSettingsRs>,
) {
    mpsc::channel()
}

impl PlatformRs {
    pub fn start(&mut self) {
        println!("Starting evdev-rs platform");

        let bridge = self.bridge.clone();
        let device_registry = self.device_registry.clone();
        let (rfd, wfd) = nix::unistd::pipe().unwrap();
        let (tx, rx) = create_thread_command_channel();

        self.wfd = Some(wfd);
        self.tx = Some(tx);
        self.handle = Some(thread::spawn(move || {
            PlatformRs::run(bridge, device_registry, rfd, rx);
        }));
    }

    pub fn continue_after_config(&self) {}
    pub fn pause_for_config(&self) {}
    pub fn stop(&self) {}
    pub fn create_device_observer(&self) -> Box<DeviceObserverRs> {
        Box::new(DeviceObserverRs::new())
    }

    fn create_input_device(&self, device_id: i32) -> Box<InputDeviceRs> {
        Box::new(InputDeviceRs {
            device_id: device_id,
            wfd: self.wfd.as_ref().unwrap().as_fd(),
            tx: self.tx.clone().unwrap(),
        })
    }

    fn run(
        bridge_locked: SharedPtr<PlatformBridgeC>,
        device_registry: SharedPtr<InputDeviceRegistry>,
        rfd: OwnedFd,
        rx: std::sync::mpsc::Receiver<ThreadCommand>,
    ) {
        let mut state = LibinputState {
            libinput: Libinput::new_with_udev(LibinputInterfaceImpl {
                bridge: bridge_locked.clone(),
                fds: Vec::new(),
            }),
            known_devices: Vec::new(),
            next_device_id: 0,
            button_state: 0,
            pointer_x: 0.0,
            pointer_y: 0.0,
            scroll_axis_x_accum: 0.0,
            scroll_axis_y_accum: 0.0,
            x_scroll_scale: 1.0,
            y_scroll_scale: 1.0,
        };

        fn get_scroll_axis(
            value120: f64,
            value: f64,
            scale: f64,
            accum: &mut f64,
        ) -> (f64, f64, f64, bool) {
            let precise = value * scale;
            let stop = precise == 0.0;
            *accum = *accum + value120;

            let discrete = *accum / 120.0;
            *accum = *accum % 120.0;
            return (precise, discrete, value120, stop);
        }

        state.libinput.udev_assign_seat("seat0").unwrap();

        let mut fds = [
            pollfd {
                fd: state.libinput.as_raw_fd(),
                events: POLLIN,
                revents: 0,
            },
            pollfd {
                fd: rfd.as_raw_fd(),
                events: POLLIN,
                revents: 0,
            },
        ];

        loop {
            unsafe {
                let ret = poll(fds.as_mut_ptr(), fds.len() as _, -1); // -1 = wait indefinitely
                if ret < 0 {
                    println!("Error polling libinput fd");
                    return;
                }
            }

            if (fds[0].revents & POLLIN) != 0 {
                if state.libinput.dispatch().is_err() {
                    // TODO: Report the error to Mir's logging facilities somehow
                    println!("Error dispatching libinput events");
                    return;
                }

                for event in &mut state.libinput {
                    match event {
                        Event::Device(device_event) => match device_event {
                            DeviceEvent::Added(added_event) => {
                                let dev: Device = added_event.device();
                                state.known_devices.push(DeviceWrapper {
                                    id: state.next_device_id,
                                    device: dev,
                                    input_device: bridge_locked
                                        .create_input_device(state.next_device_id),
                                    input_sink: None,
                                    event_builder: None,
                                });

                                // The device registry may call back into the input device, but the input device
                                // may call back into this thread to retrieve information. To avoid deadlocks, we
                                // have to queue this work onto a new thread to fire and forget it. It's not a big
                                // deal, but it is a bit unfortunate.
                                let input_device =
                                    state.known_devices.last().unwrap().input_device.clone();
                                let device_registry = device_registry.clone();
                                thread::spawn(move || unsafe {
                                    device_registry
                                        .clone()
                                        .pin_mut_unchecked()
                                        .add_device(&input_device);
                                });

                                state.next_device_id += 1;
                            }
                            DeviceEvent::Removed(removed_event) => {
                                let dev: Device = removed_event.device();
                                let index = state
                                    .known_devices
                                    .iter()
                                    .position(|x| x.device.as_raw() == dev.as_raw());
                                if let Some(index) = index {
                                    unsafe {
                                        device_registry.clone().pin_mut_unchecked().remove_device(&state.known_devices[index].input_device);
                                    }
                                    state.known_devices.remove(index);
                                }
                            }
                            _ => {}
                        },

                        // TODO: Need to set up reporting events (report->received_event_from_kernel)
                        Event::Pointer(pointer_event) => {
                            let dev: Device = pointer_event.device();

                            if let Some(device_wrapper) = state
                                .known_devices
                                .iter_mut()
                                .find(|x| x.device.as_raw() == dev.as_raw())
                            {
                                let mut event_to_publish: Option<SharedPtr<MirEvent>> = None;
                                match pointer_event {
                                    PointerEvent::Motion(motion_event) => {
                                        if let Some(event_builder) =
                                            &mut device_wrapper.event_builder
                                        {
                                            event_to_publish =
                                                Some(event_builder.pin_mut().pointer_event(
                                                    true,
                                                    motion_event.time_usec(),
                                                    MirPointerAction::Motion as i32,
                                                    state.button_state,
                                                    false,
                                                    0 as f32,
                                                    0 as f32,
                                                    motion_event.dx() as f32,
                                                    motion_event.dy() as f32,
                                                    MirPointerAxisSource::None as i32,
                                                    0.0,
                                                    0,
                                                    0,
                                                    false,
                                                    0.0,
                                                    0,
                                                    0,
                                                    false,
                                                ));
                                        }
                                    }

                                    PointerEvent::MotionAbsolute(absolute_motion_event) => {
                                        if let Some(event_builder) =
                                            &mut device_wrapper.event_builder
                                        {
                                            if let Some(input_sink) = &mut device_wrapper.input_sink
                                            {
                                                let sink_ref: &InputSink =
                                                    unsafe { input_sink.0.as_ref() };
                                                let bounding_rect =
                                                    bridge_locked.bounding_rectangle(sink_ref);
                                                let width = bounding_rect.width() as u32;
                                                let height = bounding_rect.height() as u32;

                                                let old_x = state.pointer_x;
                                                let old_y = state.pointer_y;
                                                state.pointer_x = absolute_motion_event
                                                    .absolute_x_transformed(width)
                                                    as f32;
                                                state.pointer_y = absolute_motion_event
                                                    .absolute_y_transformed(height)
                                                    as f32;

                                                let movement_x = state.pointer_x - old_x;
                                                let movement_y = state.pointer_y - old_y;

                                                event_to_publish =
                                                    Some(event_builder.pin_mut().pointer_event(
                                                        true,
                                                        absolute_motion_event.time_usec(),
                                                        MirPointerAction::Motion as i32,
                                                        state.button_state,
                                                        true,
                                                        state.pointer_x,
                                                        state.pointer_y,
                                                        movement_x,
                                                        movement_y,
                                                        MirPointerAxisSource::None as i32,
                                                        0.0,
                                                        0,
                                                        0,
                                                        false,
                                                        0.0,
                                                        0,
                                                        0,
                                                        false,
                                                    ));
                                            }
                                        }
                                    }

                                    PointerEvent::ScrollWheel(scroll_wheel_event) => {
                                        if let Some(event_builder) =
                                            &mut device_wrapper.event_builder
                                        {
                                            let (precise_x, discrete_x, value120_x, stop_x) =
                                                if scroll_wheel_event
                                                    .has_axis(input::event::pointer::Axis::Vertical)
                                                {
                                                    let scroll_value120_x = scroll_wheel_event
                                                        .scroll_value_v120(
                                                            input::event::pointer::Axis::Horizontal,
                                                        );
                                                    let scroll_value_x = scroll_wheel_event
                                                        .scroll_value(
                                                            input::event::pointer::Axis::Horizontal,
                                                        );
                                                    get_scroll_axis(
                                                        scroll_value120_x,
                                                        scroll_value_x,
                                                        state.x_scroll_scale as f64,
                                                        &mut state.scroll_axis_x_accum,
                                                    )
                                                } else {
                                                    (0.0, 0.0, 0.0, true)
                                                };

                                            let (precise_y, discrete_y, value120_y, stop_y) =
                                                if scroll_wheel_event
                                                    .has_axis(input::event::pointer::Axis::Vertical)
                                                {
                                                    let scroll_value120_y = scroll_wheel_event
                                                        .scroll_value_v120(
                                                            input::event::pointer::Axis::Vertical,
                                                        );
                                                    let scroll_value_y = scroll_wheel_event
                                                        .scroll_value(
                                                            input::event::pointer::Axis::Vertical,
                                                        );
                                                    get_scroll_axis(
                                                        scroll_value120_y,
                                                        scroll_value_y,
                                                        state.y_scroll_scale as f64,
                                                        &mut state.scroll_axis_y_accum,
                                                    )
                                                } else {
                                                    (0.0, 0.0, 0.0, true)
                                                };

                                            event_to_publish =
                                                Some(event_builder.pin_mut().pointer_event(
                                                    true,
                                                    scroll_wheel_event.time_usec(),
                                                    MirPointerAction::Motion as i32,
                                                    state.button_state,
                                                    false,
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    MirPointerAxisSource::Wheel as i32,
                                                    precise_x as f32,
                                                    discrete_x as i32,
                                                    value120_x as i32,
                                                    stop_x,
                                                    precise_y as f32,
                                                    discrete_y as i32,
                                                    value120_y as i32,
                                                    stop_y,
                                                ));
                                        }
                                    }

                                    PointerEvent::ScrollContinuous(scroll_continuous_event) => {
                                        if let Some(event_builder) =
                                            &mut device_wrapper.event_builder
                                        {
                                            let (precise_x, discrete_x, value120_x, stop_x) =
                                                if scroll_continuous_event
                                                    .has_axis(input::event::pointer::Axis::Vertical)
                                                {
                                                    let scroll_value120_x = 0.0;
                                                    let scroll_value_x = scroll_continuous_event
                                                        .scroll_value(
                                                            input::event::pointer::Axis::Horizontal,
                                                        );
                                                    get_scroll_axis(
                                                        scroll_value120_x,
                                                        scroll_value_x,
                                                        state.x_scroll_scale as f64,
                                                        &mut state.scroll_axis_x_accum,
                                                    )
                                                } else {
                                                    (0.0, 0.0, 0.0, true)
                                                };

                                            let (precise_y, discrete_y, value120_y, stop_y) =
                                                if scroll_continuous_event
                                                    .has_axis(input::event::pointer::Axis::Vertical)
                                                {
                                                    let scroll_value120_y = 0.0;
                                                    let scroll_value_y = scroll_continuous_event
                                                        .scroll_value(
                                                            input::event::pointer::Axis::Vertical,
                                                        );
                                                    get_scroll_axis(
                                                        scroll_value120_y,
                                                        scroll_value_y,
                                                        state.y_scroll_scale as f64,
                                                        &mut state.scroll_axis_y_accum,
                                                    )
                                                } else {
                                                    (0.0, 0.0, 0.0, true)
                                                };

                                            event_to_publish =
                                                Some(event_builder.pin_mut().pointer_event(
                                                    true,
                                                    scroll_continuous_event.time_usec(),
                                                    MirPointerAction::Motion as i32,
                                                    state.button_state,
                                                    false,
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    MirPointerAxisSource::Wheel as i32,
                                                    precise_x as f32,
                                                    discrete_x as i32,
                                                    value120_x as i32,
                                                    stop_x,
                                                    precise_y as f32,
                                                    discrete_y as i32,
                                                    value120_y as i32,
                                                    stop_y,
                                                ));
                                        }
                                    }

                                    PointerEvent::ScrollFinger(scroll_finger_event) => {
                                        if let Some(event_builder) =
                                            &mut device_wrapper.event_builder
                                        {
                                            let (precise_x, discrete_x, value120_x, stop_x) =
                                                if scroll_finger_event
                                                    .has_axis(input::event::pointer::Axis::Vertical)
                                                {
                                                    let scroll_value120_x = 0.0;
                                                    let scroll_value_x = scroll_finger_event
                                                        .scroll_value(
                                                            input::event::pointer::Axis::Horizontal,
                                                        );
                                                    get_scroll_axis(
                                                        scroll_value120_x,
                                                        scroll_value_x,
                                                        state.x_scroll_scale as f64,
                                                        &mut state.scroll_axis_x_accum,
                                                    )
                                                } else {
                                                    (0.0, 0.0, 0.0, true)
                                                };

                                            let (precise_y, discrete_y, value120_y, stop_y) =
                                                if scroll_finger_event
                                                    .has_axis(input::event::pointer::Axis::Vertical)
                                                {
                                                    let scroll_value120_y = 0.0;
                                                    let scroll_value_y = scroll_finger_event
                                                        .scroll_value(
                                                            input::event::pointer::Axis::Vertical,
                                                        );
                                                    get_scroll_axis(
                                                        scroll_value120_y,
                                                        scroll_value_y,
                                                        state.y_scroll_scale as f64,
                                                        &mut state.scroll_axis_y_accum,
                                                    )
                                                } else {
                                                    (0.0, 0.0, 0.0, true)
                                                };

                                            event_to_publish =
                                                Some(event_builder.pin_mut().pointer_event(
                                                    true,
                                                    scroll_finger_event.time_usec(),
                                                    MirPointerAction::Motion as i32,
                                                    state.button_state,
                                                    false,
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    MirPointerAxisSource::Wheel as i32,
                                                    precise_x as f32,
                                                    discrete_x as i32,
                                                    value120_x as i32,
                                                    stop_x,
                                                    precise_y as f32,
                                                    discrete_y as i32,
                                                    value120_y as i32,
                                                    stop_y,
                                                ));
                                        }
                                    }

                                    PointerEvent::Button(button_event) => {
                                        if let Some(event_builder) =
                                            &mut device_wrapper.event_builder
                                        {
                                            const BTN_LEFT: u32 = 0x110;
                                            const BTN_RIGHT: u32 = 0x111;
                                            const BTN_MIDDLE: u32 = 0x112;
                                            const BTN_SIDE: u32 = 0x113;
                                            const BTN_EXTRA: u32 = 0x114;
                                            const BTN_FORWARD: u32 = 0x115;
                                            const BTN_BACK: u32 = 0x116;
                                            const BTN_TASK: u32 = 0x117;

                                            let mut mir_button = match button_event.button() {
                                                BTN_LEFT => MirPointerButton::Primary,
                                                BTN_RIGHT => MirPointerButton::Secondary,
                                                BTN_MIDDLE => MirPointerButton::Tertiary,
                                                BTN_BACK => MirPointerButton::Back,
                                                BTN_FORWARD => MirPointerButton::Forward,
                                                BTN_SIDE => MirPointerButton::Side,
                                                BTN_EXTRA => MirPointerButton::Extra,
                                                BTN_TASK => MirPointerButton::Task,
                                                _ => MirPointerButton::Primary,
                                            };

                                            if device_wrapper.device.config_left_handed()
                                            {
                                                mir_button = match mir_button {
                                                    MirPointerButton::Primary => {
                                                        MirPointerButton::Secondary
                                                    }
                                                    MirPointerButton::Secondary => {
                                                        MirPointerButton::Primary
                                                    }
                                                    _ => mir_button,
                                                };
                                            }

                                            let mut action: MirPointerAction =
                                                MirPointerAction::Actions;
                                            if button_event.button_state() == ButtonState::Pressed {
                                                state.button_state =
                                                    state.button_state | mir_button as u32;
                                                action = MirPointerAction::ButtonDown;
                                            } else {
                                                state.button_state =
                                                    state.button_state & !(mir_button as u32);
                                                action = MirPointerAction::ButtonUp;
                                            }

                                            event_to_publish =
                                                Some(event_builder.pin_mut().pointer_event(
                                                    true,
                                                    button_event.time_usec(),
                                                    action as i32,
                                                    state.button_state,
                                                    false,
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    MirPointerAxisSource::None as i32,
                                                    0.0,
                                                    0,
                                                    0,
                                                    false,
                                                    0.0,
                                                    0,
                                                    0,
                                                    false,
                                                ));
                                        }
                                    }

                                    _ => {}
                                }

                                if let Some(created) = event_to_publish {
                                    if let Some(input_sink) = &mut device_wrapper.input_sink {
                                        input_sink.handle_input(&created);
                                    }
                                }
                            }
                        }

                        Event::Keyboard(keyboard_event) => {
                            let dev: Device = keyboard_event.device();

                            if let Some(device_wrapper) = state
                                .known_devices
                                .iter_mut()
                                .find(|x| x.device.as_raw() == dev.as_raw())
                            {
                                match keyboard_event {
                                    KeyboardEvent::Key(key_event) => {
                                        let keyboard_action = match key_event.key_state() {
                                            KeyState::Pressed => MirKeyboardAction::Down,
                                            KeyState::Released => MirKeyboardAction::Up,
                                        };

                                        if let Some(event_builder) =
                                            &mut device_wrapper.event_builder
                                        {
                                            let created = event_builder.pin_mut().key_event(
                                                true,
                                                key_event.time_usec(),
                                                keyboard_action as i32,
                                                key_event.key(),
                                            );

                                            if let Some(input_sink) = &mut device_wrapper.input_sink
                                            {
                                                input_sink.handle_input(&created);
                                            }
                                        }
                                    }
                                    _ => {}
                                }
                            }
                        }

                        // TODO(mattkae): Handle touch events
                        // TODO: Otherwise, find the event, process it, and request that the input sink handle it
                        _ => {}
                    }
                }
            }

            if (fds[1].revents & POLLIN) != 0 {
                let mut buf = [0u8];
                nix::unistd::read(&rfd, &mut buf).unwrap();
                // handle commands
                while let Ok(cmd) = rx.try_recv() {
                    match cmd {
                        ThreadCommand::GetDeviceInfo(id, tx) => {
                            // For simplicity, just return info about the first device
                            if let Some(dev_with_id) =
                                state.known_devices.iter().find(|d| d.id == id)
                            {
                                let mut capabilities: u32 = 0;
                                if dev_with_id
                                    .device
                                    .has_capability(DeviceCapability::Keyboard)
                                {
                                    capabilities = capabilities
                                        | MirDeviceCapability::Keyboard as u32
                                        | MirDeviceCapability::AlphaNumeric as u32;
                                }
                                if dev_with_id.device.has_capability(DeviceCapability::Pointer) {
                                    capabilities =
                                        capabilities | MirDeviceCapability::Pointer as u32;
                                }
                                if dev_with_id.device.has_capability(DeviceCapability::Touch) {
                                    capabilities = capabilities
                                        | MirDeviceCapability::Touchpad as u32
                                        | MirDeviceCapability::Pointer as u32;
                                }

                                let info = InputDeviceInfoRs {
                                    name: dev_with_id.device.name().to_string(),
                                    unique_id: dev_with_id.device.name().to_string()
                                        + &dev_with_id.device.sysname().to_string()
                                        + &dev_with_id.device.id_vendor().to_string()
                                        + &dev_with_id.device.id_product().to_string(),
                                    capabilities: capabilities,
                                };
                                let _ = tx.send(info);
                            } else {
                                let info = InputDeviceInfoRs {
                                    name: "".to_string(),
                                    unique_id: "".to_string(),
                                    capabilities: 0,
                                };
                                let _ = tx.send(info);
                            }
                        }
                        ThreadCommand::Start(id, input_sink, event_builder) => {
                            if let Some(dev_with_id) =
                                state.known_devices.iter_mut().find(|d| d.id == id)
                            {
                                dev_with_id.input_sink = Some(input_sink);
                                unsafe {
                                    dev_with_id.event_builder = Some(
                                        bridge_locked
                                            .create_event_builder_wrapper(event_builder.0.as_ptr()),
                                    );
                                }
                                println!("Starting input device with id: {}", id);
                            }
                        }
                        ThreadCommand::GetPointerSettings(id, tx) => {
                            if let Some(dev_with_id) =
                                state.known_devices.iter().find(|d| d.id == id)
                            {
                                match dev_with_id.device.has_capability(DeviceCapability::Pointer) {
                                    true => {
                                        let handedness = if dev_with_id.device.config_left_handed()
                                        {
                                            MirPointerHandedness::LeftHanded as i32
                                        } else {
                                            MirPointerHandedness::RightHanded as i32
                                        };

                                        let acceleration = if let Some(accel_profile) =
                                            dev_with_id.device.config_accel_profile()
                                        {
                                            if accel_profile == input::AccelProfile::Adaptive {
                                                MirPointerAcceleration::Adaptive as i32
                                            } else {
                                                MirPointerAcceleration::None as i32
                                            }
                                        } else {
                                            MirPointerAcceleration::None as i32
                                        };

                                        let acceleration_bias =
                                            dev_with_id.device.config_accel_speed() as f64;

                                        let settings = ffi::PointerSettingsRs {
                                            is_set: true,
                                            handedness: handedness,
                                            cursor_acceleration_bias: acceleration_bias,
                                            acceleration: acceleration,
                                            horizontal_scroll_scale: state.x_scroll_scale,
                                            vertical_scroll_scale: state.y_scroll_scale,
                                        };
                                        let _ = tx.send(settings);
                                    }
                                    false => {
                                        let settings = ffi::PointerSettingsRs::empty();
                                        let _ = tx.send(settings);
                                    }
                                }
                            } else {
                                let settings = ffi::PointerSettingsRs::empty();
                                let _ = tx.send(settings);
                            }
                        }
                        ThreadCommand::SetPointerSettings(id, settings) => {
                            if let Some(dev_with_id) =
                                state.known_devices.iter_mut().find(|d| d.id == id)
                            {
                                if dev_with_id.device.has_capability(DeviceCapability::Pointer) {
                                    let left_handed = match settings.handedness {
                                        x if x == MirPointerHandedness::LeftHanded as i32 => true,
                                        _ => false,
                                    };
                                    dev_with_id.device.config_left_handed_set(left_handed).unwrap();

                                    let accel_profile = match settings.acceleration {
                                        x if x == MirPointerAcceleration::Adaptive as i32 => input::AccelProfile::Adaptive,
                                        _ => input::AccelProfile::Flat,
                                    };
                                    dev_with_id.device.config_accel_set_profile(accel_profile).unwrap();
                                    dev_with_id.device.config_accel_set_speed(settings.cursor_acceleration_bias as f64).unwrap();
                                    state.x_scroll_scale = settings.horizontal_scroll_scale;
                                    state.y_scroll_scale = settings.vertical_scroll_scale;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

pub struct DeviceObserverRs {
    fd: Option<i32>,
}

impl DeviceObserverRs {
    pub fn new() -> Self {
        DeviceObserverRs { fd: None }
    }

    pub fn activated(&mut self, fd: i32) {
        println!("Device activated with fd: {}", fd);
        self.fd = Some(fd);
    }

    pub fn suspended(&mut self) {
        println!("Device suspended");
    }

    pub fn removed(&mut self) {
        println!("Device removed");
        self.fd = None;
    }
}

pub struct InputDeviceRs<'fd> {
    device_id: i32,
    wfd: BorrowedFd<'fd>,
    tx: mpsc::Sender<ThreadCommand>,
}

impl<'fd> InputDeviceRs<'fd> {
    pub fn start(&mut self, input_sink: *mut InputSink, event_builder: *mut EventBuilder) {
        self.tx
            .send(ThreadCommand::Start(
                self.device_id,
                InputSinkPtr(NonNull::new(input_sink).unwrap()),
                EventBuilderPtr(NonNull::new(event_builder).unwrap()),
            ))
            .unwrap();
        nix::unistd::write(&self.wfd, &[0u8]).unwrap();
    }

    pub fn stop(&mut self) {}

    pub fn get_device_info(&self) -> Box<InputDeviceInfoRs> {
        let (tx, rx) = create_device_info_rs_channel();
        self.tx
            .send(ThreadCommand::GetDeviceInfo(self.device_id, tx))
            .unwrap();
        nix::unistd::write(&self.wfd, &[0u8]).unwrap();
        let device_info = rx.recv().unwrap();
        Box::new(device_info)
    }

    pub fn get_pointer_settings(&self) -> Box<ffi::PointerSettingsRs> {
        let (tx, rx) = create_pointer_settings_rs_channel();
        self.tx
            .send(ThreadCommand::GetPointerSettings(self.device_id, tx))
            .unwrap();
        nix::unistd::write(&self.wfd, &[0u8]).unwrap();
        let pointer_settings = rx.recv().unwrap();
        Box::new(pointer_settings)
    }

    pub fn set_pointer_settings(&self, settings: &ffi::PointerSettingsC) {
        self.tx
            .send(ThreadCommand::SetPointerSettings(
                self.device_id,
                settings.clone(),
            ))
            .unwrap();
        nix::unistd::write(&self.wfd, &[0u8]).unwrap();
    }
}

impl ffi::PointerSettingsRs {
    pub fn empty() -> Self {
        ffi::PointerSettingsRs {
            is_set: false,
            handedness: 0,
            cursor_acceleration_bias: 0.0,
            acceleration: MirPointerAcceleration::None as i32,
            horizontal_scroll_scale: 1.0,
            vertical_scroll_scale: 1.0,
        }
    }
}

#[cxx::bridge(namespace = "mir::input::evdev_rs")]
mod ffi {
    #[derive(Copy, Clone)]
    struct PointerSettingsC {
        pub handedness: i32,
        pub cursor_acceleration_bias: f64,
        pub acceleration: i32,
        pub horizontal_scroll_scale: f64,
        pub vertical_scroll_scale: f64,
    }

    #[derive(Copy, Clone)]
    struct PointerSettingsRs {
        pub is_set: bool,
        pub handedness: i32,
        pub cursor_acceleration_bias: f64,
        pub acceleration: i32,
        pub horizontal_scroll_scale: f64,
        pub vertical_scroll_scale: f64,
    }

    extern "Rust" {
        type PlatformRs;
        type DeviceObserverRs;
        type InputDeviceRs<'fd>;
        type InputDeviceInfoRs;

        fn start(self: &mut PlatformRs);
        fn continue_after_config(self: &PlatformRs);
        fn pause_for_config(self: &PlatformRs);
        fn stop(self: &PlatformRs);
        fn create_device_observer(self: &PlatformRs) -> Box<DeviceObserverRs>;
        fn create_input_device(self: &PlatformRs, device_id: i32) -> Box<InputDeviceRs>;

        fn activated(self: &mut DeviceObserverRs, fd: i32);
        fn suspended(self: &mut DeviceObserverRs);
        fn removed(self: &mut DeviceObserverRs);

        unsafe fn start(
            self: &mut InputDeviceRs,
            input_sink: *mut InputSink,
            event_builder: *mut EventBuilder,
        );
        fn stop(self: &mut InputDeviceRs);
        fn get_device_info(self: &InputDeviceRs) -> Box<InputDeviceInfoRs>;

        fn name(self: &InputDeviceInfoRs) -> &str;
        fn unique_id(self: &InputDeviceInfoRs) -> &str;
        fn capabilities(self: &InputDeviceInfoRs) -> u32;
        fn get_pointer_settings(self: &InputDeviceRs) -> Box<PointerSettingsRs>;
        fn set_pointer_settings(self: &InputDeviceRs, settings: &PointerSettingsC);

        fn evdev_rs_create(
            bridge: SharedPtr<PlatformBridgeC>,
            device_registry: SharedPtr<InputDeviceRegistry>,
        ) -> Box<PlatformRs>;
    }

    unsafe extern "C++" {
        include!("platform_bridge.h");
        include!("mir/input/input_device_registry.h");
        include!("mir/input/device.h");
        include!("mir/input/input_sink.h");
        include!("mir/input/event_builder.h");
        include!("mir/events/event.h");
        include!("mir/events/scroll_axis.h");
        include!("mir_toolkit/events/enums.h");

        type PlatformBridgeC;
        type DeviceBridgeC;
        type EventBuilderWrapper;
        type RectangleC;

        #[namespace = "mir::input"]
        type Device;

        #[namespace = "mir::input"]
        type InputDevice;

        #[namespace = "mir::input"]
        type InputDeviceRegistry;

        #[namespace = "mir::input"]
        type InputSink;

        #[namespace = "mir::input"]
        type EventBuilder;

        #[namespace = ""]
        type MirEvent;

        fn acquire_device(
            self: &PlatformBridgeC,
            major: i32,
            minor: i32,
        ) -> UniquePtr<DeviceBridgeC>;
        fn bounding_rectangle(self: &PlatformBridgeC, sink: &InputSink) -> UniquePtr<RectangleC>;
        fn create_input_device(self: &PlatformBridgeC, device_id: i32) -> SharedPtr<InputDevice>;
        fn raw_fd(self: &DeviceBridgeC) -> i32;
        unsafe fn create_event_builder_wrapper(
            self: &PlatformBridgeC,
            event_builder: *mut EventBuilder,
        ) -> UniquePtr<EventBuilderWrapper>;

        // CXX-Rust doesn't support passing Option<T> to C++ functions, so I use booleans
        // instead.
        //
        // On top of this, all enums are changed to i32 for ABI stability.
        fn pointer_event(
            self: &EventBuilderWrapper,
            has_time: bool,
            time_microseconds: u64,
            action: i32,
            buttons: u32,
            has_position: bool,
            position_x: f32,
            position_y: f32,
            displacement_x: f32,
            displacement_y: f32,
            axis_source: i32,
            precise_x: f32,
            discrete_x: i32,
            value120_x: i32,
            scroll_stop_x: bool,
            precise_y: f32,
            discrete_y: i32,
            value120_y: i32,
            scroll_stop_y: bool,
        ) -> SharedPtr<MirEvent>;

        fn key_event(
            self: &EventBuilderWrapper,
            has_time: bool,
            time_microseconds: u64,
            action: i32,
            scancode: u32,
        ) -> SharedPtr<MirEvent>;

        #[namespace = "mir::input"]
        fn add_device(
            self: Pin<&mut InputDeviceRegistry>,
            device: &SharedPtr<InputDevice>,
        ) -> WeakPtr<Device>;

        #[namespace = "mir::input"]
        fn remove_device(self: Pin<&mut InputDeviceRegistry>, device: &SharedPtr<InputDevice>);

        #[namespace = "mir::input"]
        fn handle_input(self: Pin<&mut InputSink>, event: &SharedPtr<MirEvent>);

        fn x(self: &RectangleC) -> i32;
        fn y(self: &RectangleC) -> i32;
        fn width(self: &RectangleC) -> i32;
        fn height(self: &RectangleC) -> i32;
    }
}

pub use ffi::DeviceBridgeC;
pub use ffi::EventBuilder;
pub use ffi::EventBuilderWrapper;
pub use ffi::InputDevice;
pub use ffi::InputDeviceRegistry;
pub use ffi::InputSink;
pub use ffi::MirEvent;
pub use ffi::PlatformBridgeC;

pub fn evdev_rs_create(
    bridge: SharedPtr<PlatformBridgeC>,
    device_registry: SharedPtr<InputDeviceRegistry>,
) -> Box<PlatformRs> {
    return Box::new(PlatformRs {
        bridge: bridge,
        device_registry: device_registry,
        handle: None,
        wfd: None,
        tx: None,
    });
}
