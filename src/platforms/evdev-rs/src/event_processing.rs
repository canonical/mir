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

use crate::device::{DeviceInfo, InputSinkPtr, LibinputLoopState};
use crate::ffi::PointerEventDataRs;
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
use std::thread;

fn handle_input(input_sink: &mut Option<InputSinkPtr>, event: cxx::SharedPtr<crate::MirEvent>) {
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

fn get_device_info_from_libinput_device<'a>(
    known_devices: &'a mut Vec<DeviceInfo>,
    libinput_device: &input::Device,
) -> Option<&'a mut DeviceInfo> {
    known_devices
        .iter_mut()
        .find(|d| d.device.as_raw() == libinput_device.as_raw())
}

pub fn process_libinput_events(
    state: &mut LibinputLoopState,
    device_registry: cxx::SharedPtr<crate::InputDeviceRegistry>,
    bridge: cxx::SharedPtr<crate::PlatformBridge>,
) {
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
                        button_state: 0,
                        pointer_x: 0.0,
                        pointer_y: 0.0,
                    });

                    // The device registry may call back into the input device, but the input device
                    // may call back into this thread to retrieve information. To avoid deadlocks, we
                    // have to queue this work onto a new thread to fire and forget it. It's not a big
                    // deal, but it is a bit unfortunate.
                    let input_device = state.known_devices.last().unwrap().input_device.clone();
                    let device_registry = device_registry.clone();

                    // # Safety
                    //
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
                        // # Safety
                        //
                        // Because we need to use pin_mut_unchecked, this is unsafe.
                        unsafe {
                            device_registry
                                .clone()
                                .pin_mut_unchecked()
                                .remove_device(&state.known_devices[index].input_device);
                        }
                        state.known_devices.remove(index);
                    }
                }
                _ => println!("TODO: Unhandled device event type"),
            },

            input::Event::Pointer(pointer_event) => {
                if let Some(device_info) =
                    get_device_info_from_libinput_device(&mut state.known_devices, &libinput_device)
                {
                    match pointer_event {
                        event::PointerEvent::Motion(motion_event) => {
                            if let Some(event_builder) = &mut device_info.event_builder {
                                let pointer_event = PointerEventDataRs {
                                    has_time: true,
                                    time_microseconds: motion_event.time_usec(),
                                    action: crate::MirPointerAction::mir_pointer_action_motion.repr,
                                    buttons: device_info.button_state,
                                    has_position: false,
                                    position_x: 0 as f32,
                                    position_y: 0 as f32,
                                    displacement_x: motion_event.dx() as f32,
                                    displacement_y: motion_event.dy() as f32,
                                    axis_source:
                                        crate::MirPointerAxisSource::mir_pointer_axis_source_none
                                            .repr,
                                    precise_x: 0.0,
                                    discrete_x: 0,
                                    value120_x: 0,
                                    scroll_stop_x: false,
                                    precise_y: 0.0,
                                    discrete_y: 0,
                                    value120_y: 0,
                                    scroll_stop_y: false,
                                };
                                handle_input(
                                    &mut device_info.input_sink,
                                    event_builder.pin_mut().pointer_event(&pointer_event),
                                );
                            }
                        }

                        event::PointerEvent::MotionAbsolute(absolute_motion_event) => {
                            if let Some(event_builder) = &mut device_info.event_builder {
                                if let Some(input_sink) = &mut device_info.input_sink {
                                    // # Safety
                                    //
                                    // Calling as_ref on a NonNull is unsafe.
                                    let sink_ref: &crate::InputSink =
                                        unsafe { input_sink.0.as_ref() };
                                    let bounding_rect = bridge.bounding_rectangle(sink_ref);
                                    let width = bounding_rect.width() as u32;
                                    let height = bounding_rect.height() as u32;

                                    let old_x = device_info.pointer_x;
                                    let old_y = device_info.pointer_y;
                                    device_info.pointer_x =
                                        absolute_motion_event.absolute_x_transformed(width) as f32;
                                    device_info.pointer_y =
                                        absolute_motion_event.absolute_y_transformed(height) as f32;

                                    let movement_x = device_info.pointer_x - old_x;
                                    let movement_y = device_info.pointer_y - old_y;
                                    let pointer_event = PointerEventDataRs {
                                        has_time: true,
                                        time_microseconds: absolute_motion_event.time_usec(),
                                        action: crate::MirPointerAction::mir_pointer_action_motion.repr,
                                        buttons: device_info.button_state,
                                        has_position: true,
                                        position_x: device_info.pointer_x,
                                        position_y: device_info.pointer_y,
                                        displacement_x: movement_x,
                                        displacement_y: movement_y,
                                        axis_source: crate::MirPointerAxisSource::mir_pointer_axis_source_none.repr,
                                        precise_x: 0.0,
                                        discrete_x: 0,
                                        value120_x: 0,
                                        scroll_stop_x: false,
                                        precise_y: 0.0,
                                        discrete_y: 0,
                                        value120_y: 0,
                                        scroll_stop_y: false,
                                    };
                                    handle_input(
                                        &mut device_info.input_sink,
                                        event_builder.pin_mut().pointer_event(&pointer_event),
                                    );
                                }
                            }
                        }

                        event::PointerEvent::ScrollWheel(scroll_wheel_event) => {
                            if let Some(event_builder) = &mut device_info.event_builder {
                                let (precise_x, discrete_x, value120_x, stop_x) =
                                    if scroll_wheel_event
                                        .has_axis(input::event::pointer::Axis::Horizontal)
                                    {
                                        let scroll_value120_x = scroll_wheel_event
                                            .scroll_value_v120(
                                                input::event::pointer::Axis::Horizontal,
                                            );
                                        let scroll_value_x = scroll_wheel_event
                                            .scroll_value(input::event::pointer::Axis::Horizontal);
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
                                            .scroll_value(input::event::pointer::Axis::Vertical);
                                        get_scroll_axis(
                                            scroll_value120_y,
                                            scroll_value_y,
                                            state.y_scroll_scale as f64,
                                            &mut state.scroll_axis_y_accum,
                                        )
                                    } else {
                                        (0.0, 0.0, 0.0, true)
                                    };

                                let pointer_event = PointerEventDataRs {
                                    has_time: true,
                                    time_microseconds: scroll_wheel_event.time_usec(),
                                    action: crate::MirPointerAction::mir_pointer_action_motion.repr,
                                    buttons: device_info.button_state,
                                    has_position: false,
                                    position_x: 0.0,
                                    position_y: 0.0,
                                    displacement_x: 0.0,
                                    displacement_y: 0.0,
                                    axis_source:
                                        crate::MirPointerAxisSource::mir_pointer_axis_source_wheel
                                            .repr,
                                    precise_x: precise_x as f32,
                                    discrete_x: discrete_x as i32,
                                    value120_x: value120_x as i32,
                                    scroll_stop_x: stop_x,
                                    precise_y: precise_y as f32,
                                    discrete_y: discrete_y as i32,
                                    value120_y: value120_y as i32,
                                    scroll_stop_y: stop_y,
                                };
                                handle_input(
                                    &mut device_info.input_sink,
                                    event_builder.pin_mut().pointer_event(&pointer_event),
                                );
                            }
                        }

                        event::PointerEvent::ScrollContinuous(scroll_continuous_event) => {
                            if let Some(event_builder) = &mut device_info.event_builder {
                                let (precise_x, discrete_x, value120_x, stop_x) =
                                    if scroll_continuous_event
                                        .has_axis(input::event::pointer::Axis::Horizontal)
                                    {
                                        let scroll_value120_x = 0.0;
                                        let scroll_value_x = scroll_continuous_event
                                            .scroll_value(input::event::pointer::Axis::Horizontal);
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
                                            .scroll_value(input::event::pointer::Axis::Vertical);
                                        get_scroll_axis(
                                            scroll_value120_y,
                                            scroll_value_y,
                                            state.y_scroll_scale as f64,
                                            &mut state.scroll_axis_y_accum,
                                        )
                                    } else {
                                        (0.0, 0.0, 0.0, true)
                                    };

                                let pointer_event = PointerEventDataRs {
                                    has_time: true,
                                    time_microseconds: scroll_continuous_event.time_usec(),
                                    action: crate::MirPointerAction::mir_pointer_action_motion.repr,
                                    buttons: device_info.button_state,
                                    has_position: false,
                                    position_x: 0.0,
                                    position_y: 0.0,
                                    displacement_x: 0.0,
                                    displacement_y: 0.0,
                                    axis_source:
                                        crate::MirPointerAxisSource::mir_pointer_axis_source_wheel
                                            .repr,
                                    precise_x: precise_x as f32,
                                    discrete_x: discrete_x as i32,
                                    value120_x: value120_x as i32,
                                    scroll_stop_x: stop_x,
                                    precise_y: precise_y as f32,
                                    discrete_y: discrete_y as i32,
                                    value120_y: value120_y as i32,
                                    scroll_stop_y: stop_y,
                                };
                                handle_input(
                                    &mut device_info.input_sink,
                                    event_builder.pin_mut().pointer_event(&pointer_event),
                                );
                            }
                        }

                        event::PointerEvent::ScrollFinger(scroll_finger_event) => {
                            if let Some(event_builder) = &mut device_info.event_builder {
                                let (precise_x, discrete_x, value120_x, stop_x) =
                                    if scroll_finger_event
                                        .has_axis(input::event::pointer::Axis::Horizontal)
                                    {
                                        let scroll_value120_x = 0.0;
                                        let scroll_value_x = scroll_finger_event
                                            .scroll_value(input::event::pointer::Axis::Horizontal);
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
                                            .scroll_value(input::event::pointer::Axis::Vertical);
                                        get_scroll_axis(
                                            scroll_value120_y,
                                            scroll_value_y,
                                            state.y_scroll_scale as f64,
                                            &mut state.scroll_axis_y_accum,
                                        )
                                    } else {
                                        (0.0, 0.0, 0.0, true)
                                    };

                                let pointer_event = PointerEventDataRs {
                                    has_time: true,
                                    time_microseconds: scroll_finger_event.time_usec(),
                                    action: crate::MirPointerAction::mir_pointer_action_motion.repr,
                                    buttons: device_info.button_state,
                                    has_position: false,
                                    position_x: 0.0,
                                    position_y: 0.0,
                                    displacement_x: 0.0,
                                    displacement_y: 0.0,
                                    axis_source:
                                        crate::MirPointerAxisSource::mir_pointer_axis_source_wheel
                                            .repr,
                                    precise_x: precise_x as f32,
                                    discrete_x: discrete_x as i32,
                                    value120_x: value120_x as i32,
                                    scroll_stop_x: stop_x,
                                    precise_y: precise_y as f32,
                                    discrete_y: discrete_y as i32,
                                    value120_y: value120_y as i32,
                                    scroll_stop_y: stop_y,
                                };
                                handle_input(
                                    &mut device_info.input_sink,
                                    event_builder.pin_mut().pointer_event(&pointer_event),
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
                                            crate::MirPointerButton::mir_pointer_button_secondary
                                        } else {
                                            crate::MirPointerButton::mir_pointer_button_primary
                                        }
                                    }
                                    BTN_RIGHT => {
                                        if device_info.device.config_left_handed() {
                                            crate::MirPointerButton::mir_pointer_button_primary
                                        } else {
                                            crate::MirPointerButton::mir_pointer_button_secondary
                                        }
                                    }
                                    BTN_MIDDLE => {
                                        crate::MirPointerButton::mir_pointer_button_tertiary
                                    }
                                    BTN_BACK => crate::MirPointerButton::mir_pointer_button_back,
                                    BTN_FORWARD => {
                                        crate::MirPointerButton::mir_pointer_button_forward
                                    }
                                    BTN_SIDE => crate::MirPointerButton::mir_pointer_button_side,
                                    BTN_EXTRA => crate::MirPointerButton::mir_pointer_button_extra,
                                    BTN_TASK => crate::MirPointerButton::mir_pointer_button_task,
                                    _ => crate::MirPointerButton::mir_pointer_button_primary,
                                };

                                let action: crate::MirPointerAction = if button_event.button_state()
                                    == pointer::ButtonState::Pressed
                                {
                                    device_info.button_state =
                                        device_info.button_state | mir_button.repr;
                                    crate::MirPointerAction::mir_pointer_action_button_down
                                } else {
                                    device_info.button_state =
                                        device_info.button_state & !(mir_button.repr);
                                    crate::MirPointerAction::mir_pointer_action_button_up
                                };

                                let pointer_event = PointerEventDataRs {
                                    has_time: true,
                                    time_microseconds: button_event.time_usec(),
                                    action: action.repr,
                                    buttons: device_info.button_state,
                                    has_position: false,
                                    position_x: 0.0,
                                    position_y: 0.0,
                                    displacement_x: 0.0,
                                    displacement_y: 0.0,
                                    axis_source:
                                        crate::MirPointerAxisSource::mir_pointer_axis_source_none
                                            .repr,
                                    precise_x: 0.0,
                                    discrete_x: 0,
                                    value120_x: 0,
                                    scroll_stop_x: false,
                                    precise_y: 0.0,
                                    discrete_y: 0,
                                    value120_y: 0,
                                    scroll_stop_y: false,
                                };
                                handle_input(
                                    &mut device_info.input_sink,
                                    event_builder.pin_mut().pointer_event(&pointer_event),
                                );
                            }
                        }

                        #[allow(deprecated)]
                        event::PointerEvent::Axis(_) => {} // Deprecated, unhandled.
                        _ => println!("TODO: Unhandled pointer event type"),
                    }
                }
            }

            input::Event::Keyboard(keyboard_event) => {
                if let Some(device_info) =
                    get_device_info_from_libinput_device(&mut state.known_devices, &libinput_device)
                {
                    match keyboard_event {
                        event::KeyboardEvent::Key(key_event) => {
                            let keyboard_action = match key_event.key_state() {
                                keyboard::KeyState::Pressed => {
                                    crate::MirKeyboardAction::mir_keyboard_action_down
                                }
                                keyboard::KeyState::Released => {
                                    crate::MirKeyboardAction::mir_keyboard_action_up
                                }
                            };

                            if let Some(event_builder) = &mut device_info.event_builder {
                                let key_event_data = crate::KeyEventData {
                                    has_time: true,
                                    time_microseconds: key_event.time_usec(),
                                    action: keyboard_action.repr,
                                    scancode: key_event.key(),
                                };
                                let created = event_builder.pin_mut().key_event(&key_event_data);

                                if let Some(input_sink) = &mut device_info.input_sink {
                                    input_sink.handle_input(&created);
                                }
                            }
                        }
                        _ => {}
                    }
                }
            }

            input::Event::Touch(_) => println!("TODO: Handle touch events"),

            input::Event::Tablet(_) => println!("TODO: Handle tablet events"),

            input::Event::TabletPad(_) => println!("TODO: Handle tablet pad events"),

            input::Event::Gesture(_) => println!("TODO: Handle gesture events"),

            input::Event::Switch(_) => println!("TODO: Handle switch events"),

            _ => println!("TODO: Unhandled libinput event type"),
        }
    }
}
