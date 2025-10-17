/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// TODO: Report errors to Mir's logging facilities
// TODO: Need to set up reporting events when received from libinput (report->received_event_from_kernel)
// TODO: Implement continue after_config and pause_for_config

// Some notes about the implementation here:
// 1. CXX-Rust doesn't support passing Option<T> to C++ functions, so I use booleans
//    instead.
// 2. All enums are changed to i32 for ABI stability

// TODO: Make platform bridge type
// TODO: Chunk everything into other files
// TODO: Investigate not using 'poll' from nix instead of libc
// TODO: Look into event builders with default values (e.g. spreading a default config)
// TODO: https://medium.com/@adetaylor/thoughts-on-pin-3092e043eb19
// TODO: Remove 'mut' keyword where not needed

use cxx;
use input;
use input::event;
use input::event::keyboard;
use input::event::keyboard::KeyboardEventTrait;
use input::event::pointer;
use input::event::pointer::PointerEventTrait;
use input::event::pointer::PointerScrollEvent;
use input::event::EventTrait;
use input::AsRaw;
use libc;
use nix::unistd;
use std::os::fd::{AsFd, AsRawFd, BorrowedFd, FromRawFd};
use std::os::unix::ffi::OsStrExt;
use std::os::unix::io;
use std::sync::mpsc;
use std::thread;

pub struct PlatformRs {
    /// The platform bridge provides access to functionality that must happen in C++.
    bridge: cxx::SharedPtr<ffi::PlatformBridgeC>,

    /// The input device registry is used for registering and unregistering input devices.
    device_registry: cxx::SharedPtr<ffi::InputDeviceRegistry>,

    /// The handle of the thread running the input loop.
    handle: Option<thread::JoinHandle<()>>,

    /// The write end of the pipe used to wake the input thread.
    wfd: Option<io::OwnedFd>,

    /// The channel used to send non-libinput commands to the input thread.
    tx: Option<mpsc::Sender<ThreadCommand>>,
}

/// Creates a platform on the rust side of things.
///
/// The `bridge` and `device_registry` parameters are provided by the C++ side of things.
/// The C++ platform simply forwards al calls to the rust implementation.
pub fn evdev_rs_create(
    bridge: cxx::SharedPtr<ffi::PlatformBridgeC>,
    device_registry: cxx::SharedPtr<ffi::InputDeviceRegistry>,
) -> Box<PlatformRs> {
    return Box::new(PlatformRs {
        bridge: bridge,
        device_registry: device_registry,
        handle: None,
        wfd: None,
        tx: None,
    });
}

impl PlatformRs {
    /// Start the input platform.
    ///
    /// This method will spawn a thread to handle input events from both
    /// libinput and Mir. The thread will run until `stop()` is called.
    pub fn start(&mut self) {
        println!("Starting the evdev-rs platform");

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

    /// Stop the input platform.
    ///
    /// This method will signal the input thread to stop and wait for it to finish.
    pub fn stop(&mut self) {
        if let Some(tx) = &self.tx {
            let _ = tx.send(ThreadCommand::Stop);
            if let Some(wfd) = &self.wfd {
                let _ = nix::unistd::write(wfd.as_fd(), &[0u8]);
            }
        }

        if let Some(handle) = self.handle.take() {
            let _ = handle.join();
        }

        if let Some(wfd) = &self.wfd {
            let _ = unistd::close(wfd.as_raw_fd());
        }
    }

    pub fn create_device_observer(&self) -> Box<DeviceObserverRs> {
        Box::new(DeviceObserverRs::new())
    }

    fn create_input_device(&self, device_id: i32) -> Box<InputDeviceRs<'_>> {
        Box::new(InputDeviceRs {
            device_id: device_id,
            wfd: self.wfd.as_ref().unwrap().as_fd(),
            tx: self.tx.clone().unwrap(),
        })
    }

    fn run(
        bridge_locked: cxx::SharedPtr<ffi::PlatformBridgeC>,
        device_registry: cxx::SharedPtr<ffi::InputDeviceRegistry>,
        rfd: io::OwnedFd,
        rx: std::sync::mpsc::Receiver<ThreadCommand>,
    ) {
        let mut state = LibinputLoopState {
            libinput: input::Libinput::new_with_udev(LibinputInterfaceImpl {
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

        state.libinput.udev_assign_seat("seat0").unwrap();

        let mut fds = [
            libc::pollfd {
                fd: state.libinput.as_raw_fd(),
                events: libc::POLLIN,
                revents: 0,
            },
            libc::pollfd {
                fd: rfd.as_raw_fd(),
                events: libc::POLLIN,
                revents: 0,
            },
        ];

        let mut is_running = true;
        while is_running {
            let ret = unsafe {
                libc::poll(fds.as_mut_ptr(), fds.len() as _, -1) // -1 = wait indefinitely
            };

            if ret < 0 {
                println!("Error polling libinput fd");
                return;
            }

            if (fds[0].revents & libc::POLLIN) != 0 {
                PlatformRs::process_libinput_events(
                    &mut state,
                    device_registry.clone(),
                    bridge_locked.clone(),
                );
            }

            if (fds[1].revents & libc::POLLIN) != 0 {
                PlatformRs::process_thread_events(
                    rfd.try_clone().unwrap(),
                    &rx,
                    &mut state,
                    &bridge_locked,
                    &mut is_running,
                );
            }
        }
    }

    fn process_libinput_events(
        state: &mut LibinputLoopState,
        device_registry: cxx::SharedPtr<ffi::InputDeviceRegistry>,
        bridge: cxx::SharedPtr<ffi::PlatformBridgeC>,
    ) {
        fn handle_input(
            input_sink: &mut Option<InputSinkPtr>,
            event: cxx::SharedPtr<ffi::MirEvent>,
        ) {
            if let Some(input_sink) = input_sink {
                input_sink.handle_input(&event);
            }
        }

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

        if state.libinput.dispatch().is_err() {
            println!("Error dispatching libinput events");
            return;
        }

        for event in &mut state.libinput {
            let libinput_device = event.device();

            match event {
                input::Event::Device(device_event) => match device_event {
                    event::DeviceEvent::Added(_) => {
                        state.known_devices.push(DeviceInfo {
                            id: state.next_device_id,
                            device: libinput_device,
                            input_device: bridge.create_input_device(state.next_device_id),
                            input_sink: None,
                            event_builder: None,
                        });

                        // The device registry may call back into the input device, but the input device
                        // may call back into this thread to retrieve information. To avoid deadlocks, we
                        // have to queue this work onto a new thread to fire and forget it. It's not a big
                        // deal, but it is a bit unfortunate.
                        let input_device = state.known_devices.last().unwrap().input_device.clone();
                        let device_registry = device_registry.clone();
                        thread::spawn(move || unsafe {
                            device_registry
                                .clone()
                                .pin_mut_unchecked()
                                .add_device(&input_device);
                        });

                        state.next_device_id += 1;
                    }
                    event::DeviceEvent::Removed(removed_event) => {
                        let dev = removed_event.device();
                        let index = state
                            .known_devices
                            .iter()
                            .position(|x| x.device.as_raw() == dev.as_raw());
                        if let Some(index) = index {
                            unsafe {
                                device_registry
                                    .clone()
                                    .pin_mut_unchecked()
                                    .remove_device(&state.known_devices[index].input_device);
                            }
                            state.known_devices.remove(index);
                        }
                    }
                    _ => {}
                },

                input::Event::Pointer(pointer_event) => {
                    if let Some(device_info) = state
                        .known_devices
                        .iter_mut()
                        .find(|x| x.device.as_raw() == libinput_device.as_raw())
                    {
                        match pointer_event {
                            event::PointerEvent::Motion(motion_event) => {
                                if let Some(event_builder) = &mut device_info.event_builder {
                                    handle_input(
                                        &mut device_info.input_sink,
                                        event_builder.pin_mut().pointer_event(
                                            true,
                                            motion_event.time_usec(),
                                            ffi::MirPointerAction::mir_pointer_action_button_up
                                                .repr,
                                            state.button_state,
                                            false,
                                            0 as f32,
                                            0 as f32,
                                            motion_event.dx() as f32,
                                            motion_event.dy() as f32,
                                            ffi::MirPointerAxisSource::mir_pointer_axis_source_none
                                                .repr,
                                            0.0,
                                            0,
                                            0,
                                            false,
                                            0.0,
                                            0,
                                            0,
                                            false,
                                        ),
                                    );
                                }
                            }

                            event::PointerEvent::MotionAbsolute(absolute_motion_event) => {
                                if let Some(event_builder) = &mut device_info.event_builder {
                                    if let Some(input_sink) = &mut device_info.input_sink {
                                        let sink_ref: &ffi::InputSink =
                                            unsafe { input_sink.0.as_ref() };
                                        let bounding_rect = bridge.bounding_rectangle(sink_ref);
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

                                        handle_input(
                                            &mut device_info.input_sink,
                                            event_builder.pin_mut().pointer_event(
                                                true,
                                                absolute_motion_event.time_usec(),
                                                ffi::MirPointerAction::mir_pointer_action_motion.repr,
                                                state.button_state,
                                                true,
                                                state.pointer_x,
                                                state.pointer_y,
                                                movement_x,
                                                movement_y,
                                                ffi::MirPointerAxisSource::mir_pointer_axis_source_none.repr,
                                                0.0,
                                                0,
                                                0,
                                                false,
                                                0.0,
                                                0,
                                                0,
                                                false,
                                            ),
                                        );
                                    }
                                }
                            }

                            event::PointerEvent::ScrollWheel(scroll_wheel_event) => {
                                if let Some(event_builder) = &mut device_info.event_builder {
                                    let (precise_x, discrete_x, value120_x, stop_x) =
                                        if scroll_wheel_event
                                            .has_axis(input::event::pointer::Axis::Vertical)
                                        {
                                            let scroll_value120_x = scroll_wheel_event
                                                .scroll_value_v120(
                                                    input::event::pointer::Axis::Horizontal,
                                                );
                                            let scroll_value_x = scroll_wheel_event.scroll_value(
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
                                            let scroll_value_y = scroll_wheel_event.scroll_value(
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

                                    handle_input(
                                        &mut device_info.input_sink,
                                        event_builder.pin_mut().pointer_event(
                                            true,
                                            scroll_wheel_event.time_usec(),
                                            ffi::MirPointerAction::mir_pointer_action_motion.repr,
                                            state.button_state,
                                            false,
                                            0.0,
                                            0.0,
                                            0.0,
                                            0.0,
                                            ffi::MirPointerAxisSource::mir_pointer_axis_source_wheel.repr,
                                            precise_x as f32,
                                            discrete_x as i32,
                                            value120_x as i32,
                                            stop_x,
                                            precise_y as f32,
                                            discrete_y as i32,
                                            value120_y as i32,
                                            stop_y,
                                        ),
                                    );
                                }
                            }

                            event::PointerEvent::ScrollContinuous(scroll_continuous_event) => {
                                if let Some(event_builder) = &mut device_info.event_builder {
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

                                    handle_input(
                                        &mut device_info.input_sink,
                                        event_builder.pin_mut().pointer_event(
                                            true,
                                            scroll_continuous_event.time_usec(),
                                            ffi::MirPointerAction::mir_pointer_action_motion.repr,
                                            state.button_state,
                                            false,
                                            0.0,
                                            0.0,
                                            0.0,
                                            0.0,
                                            ffi::MirPointerAxisSource::mir_pointer_axis_source_wheel.repr,
                                            precise_x as f32,
                                            discrete_x as i32,
                                            value120_x as i32,
                                            stop_x,
                                            precise_y as f32,
                                            discrete_y as i32,
                                            value120_y as i32,
                                            stop_y,
                                        ),
                                    );
                                }
                            }

                            event::PointerEvent::ScrollFinger(scroll_finger_event) => {
                                if let Some(event_builder) = &mut device_info.event_builder {
                                    let (precise_x, discrete_x, value120_x, stop_x) =
                                        if scroll_finger_event
                                            .has_axis(input::event::pointer::Axis::Vertical)
                                        {
                                            let scroll_value120_x = 0.0;
                                            let scroll_value_x = scroll_finger_event.scroll_value(
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
                                            let scroll_value_y = scroll_finger_event.scroll_value(
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

                                    handle_input(
                                        &mut device_info.input_sink,
                                        event_builder.pin_mut().pointer_event(
                                            true,
                                            scroll_finger_event.time_usec(),
                                            ffi::MirPointerAction::mir_pointer_action_motion.repr,
                                            state.button_state,
                                            false,
                                            0.0,
                                            0.0,
                                            0.0,
                                            0.0,
                                            ffi::MirPointerAxisSource::mir_pointer_axis_source_wheel.repr,
                                            precise_x as f32,
                                            discrete_x as i32,
                                            value120_x as i32,
                                            stop_x,
                                            precise_y as f32,
                                            discrete_y as i32,
                                            value120_y as i32,
                                            stop_y,
                                        ),
                                    );
                                }
                            }

                            event::PointerEvent::Button(button_event) => {
                                if let Some(event_builder) = &mut device_info.event_builder {
                                    const BTN_LEFT: u32 = 0x110;
                                    const BTN_RIGHT: u32 = 0x111;
                                    const BTN_MIDDLE: u32 = 0x112;
                                    const BTN_SIDE: u32 = 0x113;
                                    const BTN_EXTRA: u32 = 0x114;
                                    const BTN_FORWARD: u32 = 0x115;
                                    const BTN_BACK: u32 = 0x116;
                                    const BTN_TASK: u32 = 0x117;

                                    let mir_button = match button_event.button() {
                                        BTN_LEFT => {
                                            if device_info.device.config_left_handed() {
                                                ffi::MirPointerButton::mir_pointer_button_secondary
                                            } else {
                                                ffi::MirPointerButton::mir_pointer_button_primary
                                            }
                                        }
                                        BTN_RIGHT => {
                                            if device_info.device.config_left_handed() {
                                                ffi::MirPointerButton::mir_pointer_button_primary
                                            } else {
                                                ffi::MirPointerButton::mir_pointer_button_secondary
                                            }
                                        }
                                        BTN_MIDDLE => {
                                            ffi::MirPointerButton::mir_pointer_button_tertiary
                                        }
                                        BTN_BACK => ffi::MirPointerButton::mir_pointer_button_back,
                                        BTN_FORWARD => {
                                            ffi::MirPointerButton::mir_pointer_button_forward
                                        }
                                        BTN_SIDE => ffi::MirPointerButton::mir_pointer_button_side,
                                        BTN_EXTRA => {
                                            ffi::MirPointerButton::mir_pointer_button_extra
                                        }
                                        BTN_TASK => ffi::MirPointerButton::mir_pointer_button_task,
                                        _ => ffi::MirPointerButton::mir_pointer_button_primary,
                                    };

                                    let action: ffi::MirPointerAction = if button_event
                                        .button_state()
                                        == pointer::ButtonState::Pressed
                                    {
                                        state.button_state = state.button_state | mir_button.repr;
                                        ffi::MirPointerAction::mir_pointer_action_button_down
                                    } else {
                                        state.button_state =
                                            state.button_state & !(mir_button.repr);
                                        ffi::MirPointerAction::mir_pointer_action_button_up
                                    };

                                    handle_input(
                                        &mut device_info.input_sink,
                                        event_builder.pin_mut().pointer_event(
                                            true,
                                            button_event.time_usec(),
                                            action.repr,
                                            state.button_state,
                                            false,
                                            0.0,
                                            0.0,
                                            0.0,
                                            0.0,
                                            ffi::MirPointerAxisSource::mir_pointer_axis_source_none
                                                .repr,
                                            0.0,
                                            0,
                                            0,
                                            false,
                                            0.0,
                                            0,
                                            0,
                                            false,
                                        ),
                                    );
                                }
                            }

                            _ => {}
                        }
                    }
                }

                input::Event::Keyboard(keyboard_event) => {
                    let dev: input::Device = keyboard_event.device();

                    if let Some(device_wrapper) = state
                        .known_devices
                        .iter_mut()
                        .find(|x| x.device.as_raw() == dev.as_raw())
                    {
                        match keyboard_event {
                            event::KeyboardEvent::Key(key_event) => {
                                let keyboard_action = match key_event.key_state() {
                                    keyboard::KeyState::Pressed => {
                                        ffi::MirKeyboardAction::mir_keyboard_action_down
                                    }
                                    keyboard::KeyState::Released => {
                                        ffi::MirKeyboardAction::mir_keyboard_action_up
                                    }
                                };

                                if let Some(event_builder) = &mut device_wrapper.event_builder {
                                    let created = event_builder.pin_mut().key_event(
                                        true,
                                        key_event.time_usec(),
                                        keyboard_action.repr,
                                        key_event.key(),
                                    );

                                    if let Some(input_sink) = &mut device_wrapper.input_sink {
                                        input_sink.handle_input(&created);
                                    }
                                }
                            }
                            _ => {}
                        }
                    }
                }

                // TODO(mattkae): Make match exhaustive
                // TODO(mattkae): Handle touch events
                _ => {}
            }
        }
    }

    fn process_thread_events(
        rfd: io::OwnedFd,
        rx: &mpsc::Receiver<ThreadCommand>,
        state: &mut LibinputLoopState,
        bridge_locked: &cxx::SharedPtr<ffi::PlatformBridgeC>,
        is_running: &mut bool,
    ) {
        let mut buf = [0u8];
        nix::unistd::read(&rfd, &mut buf).unwrap();
        // handle commands
        while let Ok(cmd) = rx.try_recv() {
            match cmd {
                ThreadCommand::Stop => {
                    *is_running = false;
                }
                ThreadCommand::GetDeviceInfo(id, tx) => {
                    // For simplicity, just return info about the first device
                    if let Some(dev_with_id) = state.known_devices.iter().find(|d| d.id == id) {
                        let mut capabilities: u32 = 0;
                        if dev_with_id
                            .device
                            .has_capability(input::DeviceCapability::Keyboard)
                        {
                            capabilities = capabilities
                                | ffi::DeviceCapability::keyboard.repr
                                | ffi::DeviceCapability::alpha_numeric.repr;
                        }
                        if dev_with_id
                            .device
                            .has_capability(input::DeviceCapability::Pointer)
                        {
                            capabilities = capabilities | ffi::DeviceCapability::pointer.repr;
                        }
                        if dev_with_id
                            .device
                            .has_capability(input::DeviceCapability::Touch)
                        {
                            capabilities = capabilities
                                | ffi::DeviceCapability::touchpad.repr
                                | ffi::DeviceCapability::pointer.repr;
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
                    if let Some(dev_with_id) = state.known_devices.iter_mut().find(|d| d.id == id) {
                        dev_with_id.input_sink = Some(input_sink);
                        dev_with_id.event_builder = Some(unsafe {
                            bridge_locked.create_event_builder_wrapper(event_builder.0.as_ptr())
                        });
                    }
                }
                ThreadCommand::GetPointerSettings(id, tx) => {
                    if let Some(dev_with_id) = state.known_devices.iter().find(|d| d.id == id) {
                        match dev_with_id
                            .device
                            .has_capability(input::DeviceCapability::Pointer)
                        {
                            true => {
                                let handedness = if dev_with_id.device.config_left_handed() {
                                    ffi::MirPointerHandedness::mir_pointer_handedness_left.repr
                                } else {
                                    ffi::MirPointerHandedness::mir_pointer_handedness_right.repr
                                };

                                let acceleration = if let Some(accel_profile) =
                                    dev_with_id.device.config_accel_profile()
                                {
                                    if accel_profile == input::AccelProfile::Adaptive {
                                        ffi::MirPointerAcceleration::mir_pointer_acceleration_adaptive.repr
                                    } else {
                                        ffi::MirPointerAcceleration::mir_pointer_acceleration_none
                                            .repr
                                    }
                                } else {
                                    ffi::MirPointerAcceleration::mir_pointer_acceleration_none.repr
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
                    if let Some(dev_with_id) = state.known_devices.iter_mut().find(|d| d.id == id) {
                        if dev_with_id
                            .device
                            .has_capability(input::DeviceCapability::Pointer)
                        {
                            let left_handed = match settings.handedness {
                                x if x
                                    == ffi::MirPointerHandedness::mir_pointer_handedness_left
                                        .repr =>
                                {
                                    true
                                }
                                _ => false,
                            };
                            dev_with_id
                                .device
                                .config_left_handed_set(left_handed)
                                .unwrap();

                            let accel_profile = match settings.acceleration {
                                x if x == ffi::MirPointerAcceleration::mir_pointer_acceleration_adaptive.repr => {
                                    input::AccelProfile::Adaptive
                                }
                                _ => input::AccelProfile::Flat,
                            };
                            dev_with_id
                                .device
                                .config_accel_set_profile(accel_profile)
                                .unwrap();
                            dev_with_id
                                .device
                                .config_accel_set_speed(settings.cursor_acceleration_bias as f64)
                                .unwrap();
                            state.x_scroll_scale = settings.horizontal_scroll_scale;
                            state.y_scroll_scale = settings.vertical_scroll_scale;
                        }
                    }
                }
            }
        }
    }
}

struct LibinputInterfaceImpl {
    /// The bridge used to acquire the device.
    bridge: cxx::SharedPtr<ffi::PlatformBridgeC>,

    /// The list of opened devices.
    ///
    /// Mir expects the caller who acquired the device to keep it alive until the device
    /// is closed. To do this, we keep it in the vector.
    fds: Vec<cxx::UniquePtr<ffi::DeviceBridgeC>>,
}

impl input::LibinputInterface for LibinputInterfaceImpl {
    // This method is called whenever libinput needs to open a new device.
    fn open_restricted(&mut self, path: &std::path::Path, _: i32) -> Result<io::OwnedFd, i32> {
        let cpath = std::ffi::CString::new(path.as_os_str().as_bytes()).unwrap();

        let mut st: libc::stat = unsafe { std::mem::zeroed() };
        let ret = unsafe { libc::stat(cpath.as_ptr(), &mut st) };
        if ret != 0 {
            return Err(ret);
        }

        let major_num = libc::major(st.st_rdev);
        let minor_num = libc::minor(st.st_rdev);

        // By keeping the device referenced in the fds vector, we ensure it stays alive
        // until close_restricted is called.
        let device = self
            .bridge
            .acquire_device(major_num as i32, minor_num as i32);
        let fd = device.raw_fd();
        self.fds.push(device);

        let owned = unsafe { io::OwnedFd::from_raw_fd(fd) };
        Ok(owned)
    }

    // This method is called when libinput is done with a device.
    fn close_restricted(&mut self, fd: io::OwnedFd) {
        let fd_raw = fd.as_raw_fd();
        let _ = unistd::close(fd);
        self.fds.retain(|d| d.raw_fd() != fd_raw);
    }
}

struct LibinputLoopState {
    libinput: input::Libinput,
    known_devices: Vec<DeviceInfo>,
    next_device_id: i32,
    button_state: u32,
    pointer_x: f32,
    pointer_y: f32,
    scroll_axis_x_accum: f64,
    scroll_axis_y_accum: f64,
    x_scroll_scale: f64,
    y_scroll_scale: f64,
}

struct DeviceInfo {
    id: i32,
    device: input::Device,
    input_device: cxx::SharedPtr<ffi::InputDevice>,
    input_sink: Option<InputSinkPtr>,
    event_builder: Option<cxx::UniquePtr<ffi::EventBuilderWrapper>>,
}

// TODO: Add a brief note about the implementation here of why this should work.
// This is a bit of a hack here.
//
// The other side of our platform bridge is known by us to be thread-safe, so we can
// safely send it to another thread. However, Rust has no way of knowing that, so
// we have to assert it ourselves.
unsafe impl Send for ffi::PlatformBridgeC {}
unsafe impl Sync for ffi::PlatformBridgeC {}
unsafe impl Send for ffi::InputDeviceRegistry {}
unsafe impl Sync for ffi::InputDeviceRegistry {}
unsafe impl Send for ffi::InputDevice {}
unsafe impl Sync for ffi::InputDevice {}
unsafe impl Send for ffi::InputSink {}
unsafe impl Sync for ffi::InputSink {}
unsafe impl Send for ffi::EventBuilder {}
unsafe impl Sync for ffi::EventBuilder {}

// This is another hack.
//
// Because *mut InputSink and *mut EventBuilder are raw pointers, Rust assumes
// that they are neither Send nor Sync. However, we know that the other side of
// the pointer is actually a C++ object that is thread-safe, so we can assert
// that these pointers are Send and Sync. We cannot define Send and Sync on the
// raw types to fix the issue unfortuately, so we have to wrap them in a new type
// and define Send and Sync on that.
pub struct InputSinkPtr(std::ptr::NonNull<ffi::InputSink>);
impl InputSinkPtr {
    fn handle_input(&mut self, event: &cxx::SharedPtr<ffi::MirEvent>) {
        let pinned = unsafe {
            // Pin the raw pointer
            std::pin::Pin::new_unchecked(self.0.as_mut())
        };
        pinned.handle_input(event);
    }
}

unsafe impl Send for InputSinkPtr {}
unsafe impl Sync for InputSinkPtr {}
pub struct EventBuilderPtr(std::ptr::NonNull<ffi::EventBuilder>);
unsafe impl Send for EventBuilderPtr {}
unsafe impl Sync for EventBuilderPtr {}

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

// TODO: Each command could have a .process method
enum ThreadCommand {
    Start(i32, InputSinkPtr, EventBuilderPtr),
    GetDeviceInfo(i32, mpsc::Sender<InputDeviceInfoRs>),
    GetPointerSettings(i32, mpsc::Sender<ffi::PointerSettingsRs>),
    SetPointerSettings(i32, ffi::PointerSettingsC),
    Stop,
}

// Warning(mattkae):
//   We can't create channel inside of the PlatformRs implementation because
//   the "mod ffi" block gets confused when parsing the template, so it thinks that the
//   closing bracket is an equality operator. Moving it out to the global scope fixes
//   things.

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

pub struct DeviceObserverRs {
    fd: Option<i32>,
}

impl DeviceObserverRs {
    pub fn new() -> Self {
        DeviceObserverRs { fd: None }
    }

    pub fn activated(&mut self, fd: i32) {
        self.fd = Some(fd);
    }

    pub fn suspended(&mut self) {}

    pub fn removed(&mut self) {
        self.fd = None;
    }
}

pub struct InputDeviceRs<'fd> {
    device_id: i32,
    wfd: BorrowedFd<'fd>,
    tx: mpsc::Sender<ThreadCommand>,
}

impl<'fd> InputDeviceRs<'fd> {
    pub fn start(
        &mut self,
        input_sink: *mut ffi::InputSink,
        event_builder: *mut ffi::EventBuilder,
    ) {
        self.tx
            .send(ThreadCommand::Start(
                self.device_id,
                InputSinkPtr(std::ptr::NonNull::new(input_sink).unwrap()),
                EventBuilderPtr(std::ptr::NonNull::new(event_builder).unwrap()),
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
            acceleration: ffi::MirPointerAcceleration::mir_pointer_acceleration_none.repr,
            horizontal_scroll_scale: 1.0,
            vertical_scroll_scale: 1.0,
        }
    }
}

#[cxx::bridge(namespace = "mir::input::evdev_rs")]
mod ffi {
    // Note(mattkae): The device capability enum MUST be in its expanded
    // form instead of using a bit shift (e.g. 1 << 3) because cxx
    // currently does not support bit shifts in enums. This will result
    // in the error: "enums with non-integer literal discriminants are
    // not supported yet".
    #[repr(u32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    enum DeviceCapability {
        unknown = 0,
        pointer = 2,
        keyboard = 4,
        touchpad = 8,
        touchscreen = 16,
        gamepad = 32,
        joystick = 64,
        switch_ = 128,
        multitouch = 256,
        alpha_numeric = 512,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    enum MirKeyboardAction {
        mir_keyboard_action_up,
        mir_keyboard_action_down,
        mir_keyboard_action_repeat,
        mir_keyboard_action_modifiers,
        mir_keyboard_actions,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum MirPointerAcceleration {
        mir_pointer_acceleration_none = 1,
        mir_pointer_acceleration_adaptive = 2,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    enum MirPointerAction {
        mir_pointer_action_button_up,
        mir_pointer_action_button_down,
        mir_pointer_action_enter,
        mir_pointer_action_leave,
        mir_pointer_action_motion,
        mir_pointer_actions,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    enum MirPointerAxisSource {
        mir_pointer_axis_source_none,
        mir_pointer_axis_source_wheel,
        mir_pointer_axis_source_finger,
        mir_pointer_axis_source_continuous,
        mir_pointer_axis_source_wheel_tilt,
    }

    #[repr(u32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    enum MirPointerButton {
        mir_pointer_button_primary = 1,
        mir_pointer_button_secondary = 2,
        mir_pointer_button_tertiary = 4,
        mir_pointer_button_back = 8,
        mir_pointer_button_forward = 16,
        mir_pointer_button_side = 32,
        mir_pointer_button_extra = 64,
        mir_pointer_button_task = 128,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    enum MirPointerHandedness {
        mir_pointer_handedness_right = 0,
        mir_pointer_handedness_left = 1,
    }

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
        fn stop(self: &mut PlatformRs);
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

        #[namespace = "mir::input"]
        type DeviceCapability;

        #[namespace = ""]
        type MirKeyboardAction;

        #[namespace = ""]
        type MirPointerAcceleration;

        #[namespace = ""]
        type MirPointerAction;

        #[namespace = ""]
        type MirPointerAxisSource;

        #[namespace = ""]
        type MirPointerButton;

        #[namespace = ""]
        type MirPointerHandedness;

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
