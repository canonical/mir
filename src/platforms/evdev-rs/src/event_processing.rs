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

use crate::device::{ContactData, InputSinkPtr, LibinputDeviceInfo, LibinputDeviceState};
use crate::ffi::PointerEventData;
use crate::MirTouchAction;
use cxx::{self, UniquePtr};
use input;
use input::event;
use input::event::keyboard;
use input::event::keyboard::KeyboardEventTrait;
use input::event::pointer;
use input::event::pointer::PointerEventTrait;
use input::event::pointer::PointerScrollEvent;
use input::event::touch::{TouchEventPosition, TouchEventSlot, TouchEventTrait};
use input::event::EventTrait;
use input::AsRaw;
use std::collections::HashMap;
use std::thread;

/// Computed scroll axis values for a single axis.
struct ScrollAxisData {
    precise: f32,
    discrete: i32,
    value120: i32,
    stop: bool,
}

impl Default for ScrollAxisData {
    fn default() -> Self {
        Self {
            precise: 0.0,
            discrete: 0,
            value120: 0,
            stop: false,
        }
    }
}

fn handle_input(input_sink: &mut Option<InputSinkPtr>, event: cxx::SharedPtr<crate::MirEvent>) {
    if let Some(input_sink) = input_sink {
        input_sink.handle_input(&event);
    }
}

fn get_scroll_axis(value120: f64, value: f64, scale: f64, accum: &mut f64) -> ScrollAxisData {
    let precise = value * scale;
    let stop = precise == 0.0;
    *accum += value120;

    let discrete = *accum / 120.0;
    *accum %= 120.0;
    ScrollAxisData {
        precise: precise as f32,
        discrete: discrete as i32,
        value120: value120 as i32,
        stop,
    }
}

fn get_device_info_from_libinput_device<'a>(
    known_devices: &'a mut Vec<LibinputDeviceInfo>,
    libinput_device: &input::Device,
) -> Option<&'a mut LibinputDeviceInfo> {
    known_devices
        .iter_mut()
        .find(|d| d.device.as_raw() == libinput_device.as_raw())
}

/// Retrieve the bounding rectangle for the provided input sink.
fn get_bounding_rectangle(
    input_sink: &mut Option<InputSinkPtr>,
    bridge: &cxx::SharedPtr<crate::PlatformBridge>,
) -> cxx::UniquePtr<crate::RectangleWrapper> {
    let Some(input_sink) = input_sink else {
        return UniquePtr::null();
    };

    // # Safety
    //
    // InputSink is a C++ class, so it is inherently unsafe to be accessing
    // it directly as a reference pointer.
    let sink_ref: &crate::InputSink = unsafe { input_sink.0.as_ref() };
    bridge.bounding_rectangle(sink_ref)
}

/// Handle device added/removed events.
fn handle_device_event(
    known_devices: &mut Vec<LibinputDeviceInfo>,
    next_device_id: &mut i32,
    device_registry: &cxx::SharedPtr<crate::InputDeviceRegistry>,
    bridge: &cxx::SharedPtr<crate::PlatformBridge>,
    device_event: event::DeviceEvent,
    libinput_device: input::Device,
) {
    match device_event {
        event::DeviceEvent::Added(_) => {
            known_devices.push(LibinputDeviceInfo {
                id: *next_device_id,
                device: libinput_device,
                input_device: bridge.create_input_device(*next_device_id),
                input_sink: None,
                event_builder: None,
                button_state: 0,
                pointer_x: 0.0,
                pointer_y: 0.0,
                touch_properties: HashMap::new(),
            });

            // The device registry may call back into the input device, but the input device
            // may call back into this thread to retrieve information. To avoid deadlocks, we
            // have to queue this work onto a new thread to fire and forget it. It's not a big
            // deal, but it is a bit unfortunate.
            let input_device = known_devices.last().unwrap().input_device.clone();
            let device_registry = device_registry.clone();

            // # Safety
            //
            thread::spawn(move || unsafe {
                device_registry
                    .clone()
                    .pin_mut_unchecked()
                    .add_device(&input_device);
            });

            *next_device_id += 1;
        }
        event::DeviceEvent::Removed(removed_event) => {
            let dev = removed_event.device();
            let Some(index) = known_devices
                .iter()
                .position(|x| x.device.as_raw() == dev.as_raw())
            else {
                return;
            };

            // # Safety
            //
            // Because we need to use pin_mut_unchecked, this is unsafe.
            unsafe {
                device_registry
                    .clone()
                    .pin_mut_unchecked()
                    .remove_device(&known_devices[index].input_device);
            }
            known_devices.remove(index);
        }
        _ => println!("TODO: Unhandled device event type"),
    }
}

/// Handle relative pointer motion events.
fn handle_pointer_motion(
    device_info: &mut LibinputDeviceInfo,
    motion_event: pointer::PointerMotionEvent,
    report: &crate::InputReport,
) {
    let Some(event_builder) = &mut device_info.event_builder else {
        return;
    };

    report.received_event_from_kernel(
        motion_event.time_usec(), input_event_codes::EV_REL!(), 0, 0);

    let pointer_event = PointerEventData {
        has_time: true,
        time_microseconds: motion_event.time_usec(),
        buttons: device_info.button_state,
        displacement_x: motion_event.dx() as f32,
        displacement_y: motion_event.dy() as f32,
        ..Default::default()
    };
    handle_input(
        &mut device_info.input_sink,
        event_builder.pin_mut().pointer_event(&pointer_event),
    );
}

/// Handle absolute pointer motion events.
///
/// Returns `false` if the event loop should skip to the next event (i.e. the bounding
/// rectangle was unavailable).
fn handle_pointer_motion_absolute(
    device_info: &mut LibinputDeviceInfo,
    bridge: &cxx::SharedPtr<crate::PlatformBridge>,
    absolute_motion_event: pointer::PointerMotionAbsoluteEvent,
    report: &crate::InputReport,
) -> bool {
    let Some(event_builder) = &mut device_info.event_builder else {
        return true;
    };

    report.received_event_from_kernel(
        absolute_motion_event.time_usec(), input_event_codes::EV_ABS!(), 0, 0);

    let bounding_rect = get_bounding_rectangle(&mut device_info.input_sink, bridge);
    if bounding_rect.is_null() {
        return false;
    }

    let width = bounding_rect.width() as u32;
    let height = bounding_rect.height() as u32;

    let old_x = device_info.pointer_x;
    let old_y = device_info.pointer_y;
    device_info.pointer_x = absolute_motion_event.absolute_x_transformed(width) as f32;
    device_info.pointer_y = absolute_motion_event.absolute_y_transformed(height) as f32;

    let pointer_event = PointerEventData {
        has_time: true,
        time_microseconds: absolute_motion_event.time_usec(),
        buttons: device_info.button_state,
        has_position: true,
        position_x: device_info.pointer_x,
        position_y: device_info.pointer_y,
        displacement_x: device_info.pointer_x - old_x,
        displacement_y: device_info.pointer_y - old_y,
        ..Default::default()
    };
    handle_input(
        &mut device_info.input_sink,
        event_builder.pin_mut().pointer_event(&pointer_event),
    );
    true
}

/// Compute scroll axis data for a single axis, or return the default (stopped) value
/// if the axis is not active.
fn compute_scroll_for_axis(
    has_axis: bool,
    value120: f64,
    value: f64,
    scale: f64,
    accum: &mut f64,
) -> ScrollAxisData {
    if has_axis {
        get_scroll_axis(value120, value, scale, accum)
    } else {
        ScrollAxisData::default()
    }
}

/// Handle all scroll pointer events (wheel, continuous, and finger).
struct ScrollEventData {
    time_usec: u64,
    has_horizontal: bool,
    has_vertical: bool,
    scroll_value_x: f64,
    scroll_value_y: f64,
    value120_x: f64,
    value120_y: f64,
    axis_source: i32,
}

fn handle_pointer_scroll(
    device_info: &mut LibinputDeviceInfo,
    scroll_state: &mut ScrollState,
    data: &ScrollEventData,
    report: &crate::InputReport,
) {
    let Some(event_builder) = &mut device_info.event_builder else {
        return;
    };

    report.received_event_from_kernel(
        data.time_usec, input_event_codes::EV_REL!(), 0, 0);

    let x = compute_scroll_for_axis(
        data.has_horizontal,
        data.value120_x,
        data.scroll_value_x,
        scroll_state.x_scroll_scale,
        &mut scroll_state.x_accum,
    );
    let y = compute_scroll_for_axis(
        data.has_vertical,
        data.value120_y,
        data.scroll_value_y,
        scroll_state.y_scroll_scale,
        &mut scroll_state.y_accum,
    );

    let pointer_event = PointerEventData {
        has_time: true,
        time_microseconds: data.time_usec,
        buttons: device_info.button_state,
        axis_source: data.axis_source,
        precise_x: x.precise,
        discrete_x: x.discrete,
        value120_x: x.value120,
        scroll_stop_x: x.stop,
        precise_y: y.precise,
        discrete_y: y.discrete,
        value120_y: y.value120,
        scroll_stop_y: y.stop,
        ..Default::default()
    };
    handle_input(
        &mut device_info.input_sink,
        event_builder.pin_mut().pointer_event(&pointer_event),
    );
}

/// Handle pointer button press/release events.
fn handle_pointer_button(
    device_info: &mut LibinputDeviceInfo,
    button_event: pointer::PointerButtonEvent,
    report: &crate::InputReport,
) {
    let Some(event_builder) = &mut device_info.event_builder else {
        return;
    };

    let mir_button = match button_event.button() {
        input_event_codes::BTN_LEFT!() => {
            if device_info.device.config_left_handed() {
                crate::MirPointerButton::mir_pointer_button_secondary
            } else {
                crate::MirPointerButton::mir_pointer_button_primary
            }
        }
        input_event_codes::BTN_RIGHT!() => {
            if device_info.device.config_left_handed() {
                crate::MirPointerButton::mir_pointer_button_primary
            } else {
                crate::MirPointerButton::mir_pointer_button_secondary
            }
        }
        input_event_codes::BTN_MIDDLE!() => crate::MirPointerButton::mir_pointer_button_tertiary,
        input_event_codes::BTN_BACK!() => crate::MirPointerButton::mir_pointer_button_back,
        input_event_codes::BTN_FORWARD!() => crate::MirPointerButton::mir_pointer_button_forward,
        input_event_codes::BTN_SIDE!() => crate::MirPointerButton::mir_pointer_button_side,
        input_event_codes::BTN_EXTRA!() => crate::MirPointerButton::mir_pointer_button_extra,
        input_event_codes::BTN_TASK!() => crate::MirPointerButton::mir_pointer_button_task,
        _ => crate::MirPointerButton::mir_pointer_button_primary,
    };

    let action = if button_event.button_state() == pointer::ButtonState::Pressed {
        device_info.button_state |= mir_button.repr;
        crate::MirPointerAction::mir_pointer_action_button_down
    } else {
        device_info.button_state &= !mir_button.repr;
        crate::MirPointerAction::mir_pointer_action_button_up
    };

    report.received_event_from_kernel(
        button_event.time_usec(), input_event_codes::EV_KEY!(), mir_button.repr as i32, action.repr as i32);

    let pointer_event = PointerEventData {
        has_time: true,
        time_microseconds: button_event.time_usec(),
        action: action.repr,
        buttons: device_info.button_state,
        ..Default::default()
    };
    handle_input(
        &mut device_info.input_sink,
        event_builder.pin_mut().pointer_event(&pointer_event),
    );
}

struct ScrollState {
    x_accum: f64,
    y_accum: f64,
    x_scroll_scale: f64,
    y_scroll_scale: f64,
}

/// Handle all pointer event sub-types.
///
/// Returns `false` if the caller's event loop should `continue` to the next event
/// (used by `MotionAbsolute` when the bounding rectangle is unavailable).
fn handle_pointer_event(
    device_info: &mut LibinputDeviceInfo,
    scroll_state: &mut ScrollState,
    bridge: &cxx::SharedPtr<crate::PlatformBridge>,
    pointer_event: event::PointerEvent,
    report: &crate::InputReport,
) -> bool {
    match pointer_event {
        event::PointerEvent::Motion(motion_event) => {
            handle_pointer_motion(device_info, motion_event, report);
        }

        event::PointerEvent::MotionAbsolute(absolute_motion_event) => {
            if !handle_pointer_motion_absolute(device_info, bridge, absolute_motion_event, report) {
                return false;
            }
        }

        event::PointerEvent::ScrollWheel(ref scroll_wheel_event) => {
            let has_horizontal = scroll_wheel_event.has_axis(pointer::Axis::Horizontal);
            let has_vertical = scroll_wheel_event.has_axis(pointer::Axis::Vertical);
            handle_pointer_scroll(
                device_info,
                scroll_state,
                &ScrollEventData {
                    time_usec: scroll_wheel_event.time_usec(),
                    has_horizontal,
                    has_vertical,
                    scroll_value_x: if has_horizontal {
                        scroll_wheel_event.scroll_value(pointer::Axis::Horizontal)
                    } else {
                        0.0
                    },
                    scroll_value_y: if has_vertical {
                        scroll_wheel_event.scroll_value(pointer::Axis::Vertical)
                    } else {
                        0.0
                    },
                    value120_x: if has_horizontal {
                        scroll_wheel_event.scroll_value_v120(pointer::Axis::Horizontal)
                    } else {
                        0.0
                    },
                    value120_y: if has_vertical {
                        scroll_wheel_event.scroll_value_v120(pointer::Axis::Vertical)
                    } else {
                        0.0
                    },
                    axis_source: crate::MirPointerAxisSource::mir_pointer_axis_source_wheel.repr,
                },
                report,
            );
        }

        event::PointerEvent::ScrollContinuous(ref scroll_continuous_event) => {
            let has_horizontal = scroll_continuous_event.has_axis(pointer::Axis::Horizontal);
            let has_vertical = scroll_continuous_event.has_axis(pointer::Axis::Vertical);
            handle_pointer_scroll(
                device_info,
                scroll_state,
                &ScrollEventData {
                    time_usec: scroll_continuous_event.time_usec(),
                    has_horizontal,
                    has_vertical,
                    scroll_value_x: if has_horizontal {
                        scroll_continuous_event.scroll_value(pointer::Axis::Horizontal)
                    } else {
                        0.0
                    },
                    scroll_value_y: if has_vertical {
                        scroll_continuous_event.scroll_value(pointer::Axis::Vertical)
                    } else {
                        0.0
                    },
                    value120_x: 0.0,
                    value120_y: 0.0,
                    axis_source: crate::MirPointerAxisSource::mir_pointer_axis_source_wheel.repr,
                },
                report,
            );
        }

        event::PointerEvent::ScrollFinger(ref scroll_finger_event) => {
            let has_horizontal = scroll_finger_event.has_axis(pointer::Axis::Horizontal);
            let has_vertical = scroll_finger_event.has_axis(pointer::Axis::Vertical);
            handle_pointer_scroll(
                device_info,
                scroll_state,
                &ScrollEventData {
                    time_usec: scroll_finger_event.time_usec(),
                    has_horizontal,
                    has_vertical,
                    scroll_value_x: if has_horizontal {
                        scroll_finger_event.scroll_value(pointer::Axis::Horizontal)
                    } else {
                        0.0
                    },
                    scroll_value_y: if has_vertical {
                        scroll_finger_event.scroll_value(pointer::Axis::Vertical)
                    } else {
                        0.0
                    },
                    value120_x: 0.0,
                    value120_y: 0.0,
                    axis_source: crate::MirPointerAxisSource::mir_pointer_axis_source_wheel.repr,
                },
                report,
            );
        }

        event::PointerEvent::Button(button_event) => {
            handle_pointer_button(device_info, button_event, report);
        }

        #[allow(deprecated)]
        event::PointerEvent::Axis(_) => {} // Deprecated, unhandled.
        _ => println!("TODO: Unhandled pointer event type"),
    }

    true
}

/// Handle keyboard key press/release events.
fn handle_keyboard_event(
    device_info: &mut LibinputDeviceInfo,
    keyboard_event: event::KeyboardEvent,
    report: &crate::InputReport
) {
    let event::KeyboardEvent::Key(key_event) = keyboard_event else {
        return;
    };

    let Some(event_builder) = &mut device_info.event_builder else {
        return;
    };

    let keyboard_action = match key_event.key_state() {
        keyboard::KeyState::Pressed => crate::MirKeyboardAction::mir_keyboard_action_down,
        keyboard::KeyState::Released => crate::MirKeyboardAction::mir_keyboard_action_up,
    };

    let key_event_data = crate::KeyEventData {
        has_time: true,
        time_microseconds: key_event.time_usec(),
        action: keyboard_action.repr,
        scancode: key_event.key(),
    };

    report.received_event_from_kernel(
        key_event.time_usec(), input_event_codes::EV_KEY!(), key_event.key() as i32, keyboard_action.repr as i32);

    let created = event_builder.pin_mut().key_event(&key_event_data);

    if let Some(input_sink) = &mut device_info.input_sink {
        input_sink.handle_input(&created);
    }
}

/// Handle touch events (down, motion, up, frame).
///
/// Returns `false` if the caller's event loop should `continue` to the next event
/// (used when the bounding rectangle is unavailable or a frame should be dropped).
fn handle_touch_event(
    device_info: &mut LibinputDeviceInfo,
    bridge: &cxx::SharedPtr<crate::PlatformBridge>,
    touch_event: event::TouchEvent,
    report: &crate::InputReport
) -> bool {
    match touch_event {
        event::TouchEvent::Down(down_event) => {
            let Some(slot) = down_event.slot() else {
                return true;
            };

            let bounding = get_bounding_rectangle(&mut device_info.input_sink, bridge);
            if bounding.is_null() {
                return false;
            }

            // We make sure that we only notify of "down" touch events once. Everything
            // after that is considered a simple "change".
            let data = device_info
                .touch_properties
                .entry(slot)
                .or_insert(ContactData::default());
            data.action = if data.down_notified {
                MirTouchAction::mir_touch_action_change
            } else {
                MirTouchAction::mir_touch_action_down
            };

            // Set coordinates. In normal operation, bounding rectangle should always be valid.
            data.x = down_event.x_transformed(bounding.width() as u32) as f32;
            data.y = down_event.y_transformed(bounding.height() as u32) as f32;
        }
        event::TouchEvent::Motion(motion_event) => {
            let Some(slot) = motion_event.slot() else {
                return true;
            };

            let bounding = get_bounding_rectangle(&mut device_info.input_sink, bridge);
            if bounding.is_null() {
                return false;
            }

            let data = device_info
                .touch_properties
                .entry(slot)
                .or_insert(ContactData::default());
            data.action = MirTouchAction::mir_touch_action_change;

            // Set coordinates. In normal operation, bounding rectangle should always be valid.
            data.x = motion_event.x_transformed(bounding.width() as u32) as f32;
            data.y = motion_event.y_transformed(bounding.height() as u32) as f32;
        }
        event::TouchEvent::Up(up_event) => {
            let Some(slot) = up_event.slot() else {
                return true;
            };

            // By design, "up" is only notified a single time. Hence, we only
            // notify "up" if "down" has been sent. Otherwise, we remove the
            // entry altogether.
            if let Some(data) = device_info.touch_properties.get_mut(&slot) {
                if data.down_notified {
                    data.action = MirTouchAction::mir_touch_action_up;
                } else {
                    device_info.touch_properties.remove(&slot);
                }
            }
        }
        event::TouchEvent::Frame(frame_event) => {
            return handle_touch_frame(device_info, frame_event, report);
        }
        _ => {}
    }

    true
}

/// Handle a touch frame event, assembling all pending contacts and dispatching them.
///
/// Returns `false` if the frame should be dropped (no contacts, or bogus all-zero coordinates).
fn handle_touch_frame(
    device_info: &mut LibinputDeviceInfo,
    frame_event: input::event::touch::TouchFrameEvent,
    report: &crate::InputReport,
) -> bool {
    let mut empty_touches = 0;

    let contacts: Vec<crate::TouchContactData> = device_info
        .touch_properties
        .iter_mut()
        .map(|(slot, contact_data)| {
            // Note: This was logic taken from the existing evdev implementation, and we are
            // keeping it for backwards compatibility.
            // Sanity check: Bogus panels are sending sometimes empty events that all point
            // to (0, 0) coordinates. Detect those and drop the whole frame in this case.
            // Count touches at (0, 0) but still include them in contacts for now.
            if contact_data.x == 0.0 && contact_data.y == 0.0 {
                empty_touches += 1;
            }

            let contact = crate::TouchContactData {
                touch_id: *slot as i32,
                action: contact_data.action.repr,
                tooltype: contact_data.tooltype.repr,
                position_x: contact_data.x,
                position_y: contact_data.y,
                pressure: contact_data.pressure,
                touch_major: contact_data.major,
                touch_minor: contact_data.minor,
                orientation: contact_data.orientation,
            };

            if contact_data.action == MirTouchAction::mir_touch_action_down {
                contact_data.action = MirTouchAction::mir_touch_action_change;
                contact_data.down_notified = true;
            }

            contact
        })
        .collect();

    // Remove any property that is now "up" so that we do not keep sending
    // the up notification.
    device_info
        .touch_properties
        .retain(|&_slot, contact_data| contact_data.action != MirTouchAction::mir_touch_action_up);

    // Drop the frame if we have no contacts or more than one touch at (0, 0).
    // A single touch at (0, 0) could be legitimate (top-left corner).
    if contacts.is_empty() || empty_touches > 1 {
        return false;
    }

    let time_usec = frame_event.time_usec();
    report.received_event_from_kernel(
        time_usec, input_event_codes::EV_SYN!(), 0, 0);

    let touch_event_data = crate::TouchEventData {
        has_time: true,
        time_microseconds: time_usec,
        contacts,
    };

    let Some(event_builder) = &mut device_info.event_builder else {
        return true;
    };

    let created = event_builder.pin_mut().touch_event(&touch_event_data);
    if let Some(input_sink) = &mut device_info.input_sink {
        input_sink.handle_input(&created);
    }

    true
}

pub fn process_libinput_events(
    state: &mut LibinputDeviceState,
    device_registry: cxx::SharedPtr<crate::InputDeviceRegistry>,
    bridge: cxx::SharedPtr<crate::PlatformBridge>,
    report: &crate::InputReport
) {
    if state.libinput.dispatch().is_err() {
        println!("Error dispatching libinput events");
        return;
    }

    for event in &mut state.libinput {
        let libinput_device = event.device();

        match event {
            input::Event::Device(device_event) => {
                handle_device_event(
                    &mut state.known_devices,
                    &mut state.next_device_id,
                    &device_registry,
                    &bridge,
                    device_event,
                    libinput_device,
                );
            }

            input::Event::Pointer(pointer_event) => {
                let Some(device_info) = get_device_info_from_libinput_device(
                    &mut state.known_devices,
                    &libinput_device,
                ) else {
                    continue;
                };

                let mut scroll_state = ScrollState {
                    x_accum: state.scroll_axis_x_accum,
                    y_accum: state.scroll_axis_y_accum,
                    x_scroll_scale: state.x_scroll_scale as f64,
                    y_scroll_scale: state.y_scroll_scale as f64,
                };

                let should_continue =
                    handle_pointer_event(device_info, &mut scroll_state, &bridge, pointer_event, report);

                state.scroll_axis_x_accum = scroll_state.x_accum;
                state.scroll_axis_y_accum = scroll_state.y_accum;

                if !should_continue {
                    continue;
                }
            }

            input::Event::Keyboard(keyboard_event) => {
                let Some(device_info) = get_device_info_from_libinput_device(
                    &mut state.known_devices,
                    &libinput_device,
                ) else {
                    continue;
                };

                handle_keyboard_event(device_info, keyboard_event, &report);
            }

            input::Event::Touch(touch_event) => {
                let Some(device_info) = get_device_info_from_libinput_device(
                    &mut state.known_devices,
                    &libinput_device,
                ) else {
                    continue;
                };

                if !handle_touch_event(device_info, &bridge, touch_event, &report) {
                    continue;
                }
            }

            input::Event::Tablet(_) => println!("TODO: Handle tablet events"),

            input::Event::TabletPad(_) => println!("TODO: Handle tablet pad events"),

            input::Event::Gesture(_) => println!("TODO: Handle gesture events"),

            input::Event::Switch(_) => println!("TODO: Handle switch events"),

            _ => println!("TODO: Unhandled libinput event type"),
        }
    }
}
