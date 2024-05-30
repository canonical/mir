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

#include "libinput_device.h"
#include "libinput_ptr.h"
#include "libinput_device_ptr.h"
#include "button_utils.h"

#include "mir/input/input_sink.h"
#include "mir/input/input_report.h"
#include "mir/input/device_capability.h"
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/input_device_info.h"
#include "mir/events/event_builders.h"
#include "mir/geometry/displacement.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/fd.h"
#define MIR_LOG_COMPONENT "evdev"
#include "mir/log.h"
#include "mir/raii.h"

#include <libinput.h>
#include <linux/input.h>  // only used to get constants for input reports

#include <boost/exception/diagnostic_information.hpp>
#include <cstring>
#include <chrono>
#include <sstream>
#include <algorithm>

namespace md = mir::dispatch;
namespace mi = mir::input;
namespace mie = mi::evdev;
namespace mev = mir::events;
namespace geom = mir::geometry;
using namespace std::literals::chrono_literals;

namespace
{
// These functions were not accepted into libinput, but we would want to use them if they were.
// By introducing a potential overload ambiguity we will be notified that happens.
double libinput_event_touch_get_major_transformed(libinput_event_touch*, uint32_t, uint32_t)
{
    return 8;
}
double libinput_event_touch_get_touch_minor_transformed(libinput_event_touch*, uint32_t, uint32_t)
{
    return 6;
}
double libinput_event_touch_get_pressure(libinput_event_touch*)
{
    return 0.8;
}

template<typename Tag>
auto get_scroll_axis(
    libinput_event_pointer* event,
    libinput_pointer_axis axis,
    geom::generic::Value<int, Tag>& accumulator,
    geom::generic::Value<float, Tag> scale) -> mev::ScrollAxis<Tag>
{
    if (!libinput_event_pointer_has_axis(event, axis))
    {
        return {};
    }

    auto const libinput_event_type = libinput_event_get_type(libinput_event_pointer_get_base_event(event));

    mir::geometry::generic::Value<float, Tag> const precise{
        libinput_event_pointer_get_scroll_value(event, axis) * scale};
    auto const stop = precise.as_value() == 0;
    mir::geometry::generic::Value<int, Tag> const value120{
        libinput_event_type == LIBINPUT_EVENT_POINTER_SCROLL_WHEEL ?
        libinput_event_pointer_get_scroll_value_v120(event, axis) :
        0
    };
    accumulator += value120;
    mir::geometry::generic::Value<int, Tag> const discrete{accumulator / 120};
    accumulator = geom::generic::Value<int, Tag>{accumulator.as_value() % 120};

    return {precise, discrete, value120, stop};
}

auto get_axis_source(libinput_event_pointer* pointer) -> MirPointerAxisSource
{
    switch (libinput_event_get_type(libinput_event_pointer_get_base_event(pointer)))
    {
    case LIBINPUT_EVENT_POINTER_SCROLL_WHEEL:       return mir_pointer_axis_source_wheel;
    case LIBINPUT_EVENT_POINTER_SCROLL_FINGER:      return mir_pointer_axis_source_finger;
    case LIBINPUT_EVENT_POINTER_SCROLL_CONTINUOUS:  return mir_pointer_axis_source_continuous;
    default:                                        return mir_pointer_axis_source_none;
    }
}
}

mie::LibInputDevice::LibInputDevice(std::shared_ptr<mi::InputReport> const& report, LibInputDevicePtr dev)
    : report{report}, device_{std::move(dev)}, pointer_pos{0, 0}, button_state{0}
{
    update_device_info();
}

mie::LibInputDevice::~LibInputDevice() = default;

void mie::LibInputDevice::start(InputSink* sink, EventBuilder* builder)
{
    this->sink = sink;
    this->builder = builder;
}

void mie::LibInputDevice::stop()
{
    sink = nullptr;
    builder = nullptr;
}

void mie::LibInputDevice::process_event(libinput_event* event)
{
    if (!sink)
        return;

    try
    {
        switch(libinput_event_get_type(event))
        {
        case LIBINPUT_EVENT_KEYBOARD_KEY:
            sink->handle_input(convert_event(libinput_event_get_keyboard_event(event)));
            break;
        case LIBINPUT_EVENT_POINTER_MOTION:
            sink->handle_input(convert_motion_event(libinput_event_get_pointer_event(event)));
            break;
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
            sink->handle_input(convert_absolute_motion_event(libinput_event_get_pointer_event(event)));
            break;
        case LIBINPUT_EVENT_POINTER_BUTTON:
            sink->handle_input(convert_button_event(libinput_event_get_pointer_event(event)));
            break;
        case LIBINPUT_EVENT_POINTER_SCROLL_WHEEL:
        case LIBINPUT_EVENT_POINTER_SCROLL_FINGER:
        case LIBINPUT_EVENT_POINTER_SCROLL_CONTINUOUS:
            sink->handle_input(convert_axis_event(libinput_event_get_pointer_event(event)));
            break;
        // touch events are processed as a batch of changes over all touch pointts
        case LIBINPUT_EVENT_TOUCH_DOWN:
            handle_touch_down(libinput_event_get_touch_event(event));
            break;
        case LIBINPUT_EVENT_TOUCH_UP:
            handle_touch_up(libinput_event_get_touch_event(event));
            break;
        case LIBINPUT_EVENT_TOUCH_MOTION:
            handle_touch_motion(libinput_event_get_touch_event(event));
            break;
        case LIBINPUT_EVENT_TOUCH_CANCEL:
            // Not yet provided by libinput.
            break;
        case LIBINPUT_EVENT_TOUCH_FRAME:
            if (is_output_active())
            {
                if (auto input = convert_touch_frame(libinput_event_get_touch_event(event)))
                {
                    sink->handle_input(std::move(input));
                }
            }
            break;
        default:
            break;
        }
    }
    catch(std::exception const& error)
    {
        mir::log_error("Failure processing input event received from libinput: " + boost::diagnostic_information(error));
    }
}

mir::EventUPtr mie::LibInputDevice::convert_event(libinput_event_keyboard* keyboard)
{
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_keyboard_get_time_usec(keyboard));
    auto const action = libinput_event_keyboard_get_key_state(keyboard) == LIBINPUT_KEY_STATE_PRESSED ?
                      mir_keyboard_action_down :
                      mir_keyboard_action_up;
    auto const code = libinput_event_keyboard_get_key(keyboard);
    report->received_event_from_kernel(time.count(), EV_KEY, code, action);

    return builder->key_event(time, action, xkb_keysym_t{0}, code);
}

mir::EventUPtr mie::LibInputDevice::convert_button_event(libinput_event_pointer* pointer)
{
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_pointer_get_time_usec(pointer));
    auto const button = libinput_event_pointer_get_button(pointer);
    auto const action = (libinput_event_pointer_get_button_state(pointer) == LIBINPUT_BUTTON_STATE_PRESSED)?
        mir_pointer_action_button_down : mir_pointer_action_button_up;

    auto const do_not_swap_buttons = mir_pointer_handedness_right;
    auto const pointer_button = mie::to_pointer_button(button, do_not_swap_buttons);

    report->received_event_from_kernel(time.count(), EV_KEY, pointer_button, action);

    if (action == mir_pointer_action_button_down)
        button_state = MirPointerButton(button_state | uint32_t(pointer_button));
    else
        button_state = MirPointerButton(button_state & ~uint32_t(pointer_button));

    return builder->pointer_event(
        time,
        action,
        button_state,
        std::nullopt,
        {},
        mir_pointer_axis_source_none,
        {},
        {});
}

mir::EventUPtr mie::LibInputDevice::convert_motion_event(libinput_event_pointer* pointer)
{
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_pointer_get_time_usec(pointer));
    auto const action = mir_pointer_action_motion;

    report->received_event_from_kernel(time.count(), EV_REL, 0, 0);

    return builder->pointer_event(
        time,
        action,
        button_state,
        std::nullopt,
        geom::DisplacementF{libinput_event_pointer_get_dx(pointer), libinput_event_pointer_get_dy(pointer)},
        mir_pointer_axis_source_none,
        {},
        {});
}

mir::EventUPtr mie::LibInputDevice::convert_absolute_motion_event(libinput_event_pointer* pointer)
{
    // a pointing device that emits absolute coordinates
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_pointer_get_time_usec(pointer));
    auto const action = mir_pointer_action_motion;
    // either the bounding box .. or the specific output ..
    auto const screen = sink->bounding_rectangle();
    uint32_t const width = screen.size.width.as_int();
    uint32_t const height = screen.size.height.as_int();

    report->received_event_from_kernel(time.count(), EV_ABS, 0, 0);

    auto const old_pointer_pos = pointer_pos;
    pointer_pos = {
        libinput_event_pointer_get_absolute_x_transformed(pointer, width),
        libinput_event_pointer_get_absolute_y_transformed(pointer, height),
    };
    auto const movement = pointer_pos - old_pointer_pos;

    return builder->pointer_event(
        time,
        action,
        button_state,
        pointer_pos,
        movement,
        mir_pointer_axis_source_none,
        {},
        {});
}

mir::EventUPtr mie::LibInputDevice::convert_axis_event(libinput_event_pointer* pointer)
{
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_pointer_get_time_usec(pointer));
    auto const action = mir_pointer_action_motion;

    auto const h_scroll = get_scroll_axis(
        pointer,
        LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL,
        horizontal_scroll_value120_accum /* passed as non-const ref */,
        horizontal_scroll_scale);
    auto const v_scroll = get_scroll_axis(
        pointer,
        LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL,
        vertical_scroll_value120_accum /* passed as non-const ref */,
        vertical_scroll_scale);
    auto const axis_source = get_axis_source(pointer);

    report->received_event_from_kernel(time.count(), EV_REL, 0, 0);

    return builder->pointer_event(
        time,
        action,
        button_state,
        std::nullopt,
        {},
        axis_source,
        h_scroll,
        v_scroll);
}

mir::EventUPtr mie::LibInputDevice::convert_touch_frame(libinput_event_touch* touch)
{
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_touch_get_time_usec(touch));
    report->received_event_from_kernel(time.count(), EV_SYN, 0, 0);

    // TODO make libinput indicate tool type
    auto const tool = mir_touch_tooltype_finger;

    std::vector<events::TouchContact> contacts;
    for(auto it = begin(last_seen_properties); it != end(last_seen_properties);)
    {
        auto & id = it->first;
        auto & data = it->second;

        contacts.push_back(events::TouchContact{
                           id,
                           data.action,
                           tool,
                           {data.x, data.y},
                           data.pressure,
                           data.major,
                           data.minor,
                           data.orientation});

        if (data.action == mir_touch_action_down)
        {
            data.action = mir_touch_action_change;
            data.down_notified = true;
        }

        if (data.action == mir_touch_action_up)
            it = last_seen_properties.erase(it);
        else
            ++it;
    }

    // Sanity check: Bogus panels are sending sometimes empty events that all point
    // to (0, 0) coordinates. Detect those and drop the whole frame in this case.
    // Also drop touch frames with no contacts inside
    int emptyTouches = 0;
    for (const auto &contact: contacts)
    {
        if (contact.position == geom::PointF{})
            emptyTouches++;
    }
    if (contacts.empty() || emptyTouches > 1)
        return {nullptr, [](auto){}};

    return builder->touch_event(time, contacts);
}

void mie::LibInputDevice::handle_touch_down(libinput_event_touch* touch)
{
    MirTouchId const id = libinput_event_touch_get_slot(touch);
    auto const it = last_seen_properties.find(id);

    if (it == end(last_seen_properties) || !it->second.down_notified)
    {
        update_contact_data(last_seen_properties[id], mir_touch_action_down, touch);
    }
    else
    {
        update_contact_data(it->second, mir_touch_action_change, touch);
    }
}

void mie::LibInputDevice::handle_touch_up(libinput_event_touch* touch)
{
    MirTouchId const id = libinput_event_touch_get_slot(touch);

    //Only update last_seen_properties[].action if there is a valid record in the map
    //for this ID. Otherwise, an invalid "up" event from a bogus panel will
    //create a fake action.
    auto const it = last_seen_properties.find(id);
    if (it != end(last_seen_properties))
    {
        if (it->second.down_notified)
        {
            it->second.action = mir_touch_action_up;
        }
        else
        {
            last_seen_properties.erase(it);
        }
    }
}

void mie::LibInputDevice::update_contact_data(ContactData & data, MirTouchAction action, libinput_event_touch* touch)
{
    auto info = get_output_info();

    uint32_t width = info.output_size.width.as_int();
    uint32_t height = info.output_size.height.as_int();

    data.action = action;
    data.x = libinput_event_touch_get_x_transformed(touch, width);
    data.y = libinput_event_touch_get_y_transformed(touch, height);
    data.major = libinput_event_touch_get_major_transformed(touch, width, height);
    data.minor = libinput_event_touch_get_touch_minor_transformed(touch, width, height);
    data.pressure = libinput_event_touch_get_pressure(touch);

    info.transform_to_scene(data.x, data.y);
}

void mie::LibInputDevice::handle_touch_motion(libinput_event_touch* touch)
{
    MirTouchId const id = libinput_event_touch_get_slot(touch);
    update_contact_data(last_seen_properties[id], mir_touch_action_change, touch);
}

mi::InputDeviceInfo mie::LibInputDevice::get_device_info()
{
    return info;
}

void mie::LibInputDevice::update_device_info()
{
    auto dev = device();
    std::string name = libinput_device_get_name(dev);
    std::stringstream unique_id;
    unique_id << name << '-' << libinput_device_get_sysname(dev) << '-' <<
        libinput_device_get_id_vendor(dev) << '-' <<
        libinput_device_get_id_product(dev);
    mi::DeviceCapabilities caps;

    using Caps = mi::DeviceCapabilities;
    using Mapping = std::pair<char const*, Caps>;
    auto const mappings = {
        Mapping{"ID_INPUT_MOUSE", mi::DeviceCapability::pointer},
        Mapping{"ID_INPUT_TOUCHSCREEN", mi::DeviceCapability::touchscreen},
        Mapping{"ID_INPUT_TOUCHPAD", Caps(mi::DeviceCapability::touchpad) | mi::DeviceCapability::pointer},
        Mapping{"ID_INPUT_JOYSTICK", mi::DeviceCapability::joystick},
        Mapping{"ID_INPUT_KEY", mi::DeviceCapability::keyboard},
        Mapping{"ID_INPUT_KEYBOARD", mi::DeviceCapability::alpha_numeric},
    };

    auto const u_dev = mir::raii::deleter_for(libinput_device_get_udev_device(dev), &udev_device_unref);
    for (auto const& mapping : mappings)
    {
        int detected = 0;
        auto value = udev_device_get_property_value(u_dev.get(), mapping.first);
        if (!value)
            continue;

        std::stringstream property_value(value);
        property_value >> detected;
        if (!property_value.fail() && detected)
            caps |= mapping.second;
    }

    if (contains(caps, mi::DeviceCapability::touchscreen) &&
        !contains(info.capabilities, mi::DeviceCapability::touchscreen))
    {
        touchscreen = mi::TouchscreenSettings{};

        // FIXME: We need a way to populate output_id sensibly. {alan_g}
        // https://github.com/MirServer/mir/issues/611
        touchscreen.value().mapping_mode = mir_touchscreen_mapping_mode_to_output;
    }

    info = mi::InputDeviceInfo{name, unique_id.str(), caps};
}

libinput_device_group* mie::LibInputDevice::group()
{
    return libinput_device_get_device_group(device());
}

libinput_device* mie::LibInputDevice::device() const
{
    return device_.get();
}

mi::OutputInfo mie::LibInputDevice::get_output_info() const
{
    if (touchscreen.is_set() && touchscreen.value().mapping_mode == mir_touchscreen_mapping_mode_to_output)
    {
        return sink->output_info(touchscreen.value().output_id);
    }
    else
    {
        auto scene_bbox = sink->bounding_rectangle();
        return mi::OutputInfo(
            true,
            scene_bbox.size,
            mi::OutputInfo::Matrix{{1.0f, 0.0f, float(scene_bbox.top_left.x.as_int()),
                                    0.0f, 1.0f, float(scene_bbox.top_left.y.as_int())}});
    }
}

bool mie::LibInputDevice::is_output_active() const
{
    if (!sink)
        return false;

    if (touchscreen.is_set())
    {
        auto const& touchscreen_config = touchscreen.value();
        if (touchscreen_config.mapping_mode == mir_touchscreen_mapping_mode_to_output)
        {
            auto output = sink->output_info(touchscreen_config.output_id);
            return output.active;
        }
    }
    return true;
}

mir::optional_value<mi::PointerSettings> mie::LibInputDevice::get_pointer_settings() const
{
    if (!contains(info.capabilities, mi::DeviceCapability::pointer))
        return {};

    mi::PointerSettings settings;
    auto dev = device();
    auto const left_handed = (libinput_device_config_left_handed_get(dev) == 1);
    settings.handedness = left_handed? mir_pointer_handedness_left : mir_pointer_handedness_right;
    if (libinput_device_config_accel_get_profile(dev) == LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT)
        settings.acceleration = mir_pointer_acceleration_none;
    else
        settings.acceleration = mir_pointer_acceleration_adaptive;
    settings.cursor_acceleration_bias = libinput_device_config_accel_get_speed(dev);
    settings.vertical_scroll_scale = vertical_scroll_scale.as_value();
    settings.horizontal_scroll_scale = horizontal_scroll_scale.as_value();
    return settings;
}

void mie::LibInputDevice::apply_settings(mir::input::PointerSettings const& settings)
{
    if (!contains(info.capabilities, mi::DeviceCapability::pointer))
        return;

    auto dev = device();

    auto accel_profile = settings.acceleration == mir_pointer_acceleration_adaptive ?
        LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE :
        LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;

    libinput_device_config_accel_set_speed(dev, settings.cursor_acceleration_bias);
    libinput_device_config_left_handed_set(dev, mir_pointer_handedness_left == settings.handedness);
    vertical_scroll_scale = geom::DeltaYF{settings.vertical_scroll_scale};
    horizontal_scroll_scale = geom::DeltaXF{settings.horizontal_scroll_scale};

    libinput_device_config_accel_set_profile(dev, accel_profile);
}

mir::optional_value<mi::TouchpadSettings> mie::LibInputDevice::get_touchpad_settings() const
{
    if (!contains(info.capabilities, mi::DeviceCapability::touchpad))
        return {};

    auto dev = device();
    auto click_modes = libinput_device_config_click_get_method(dev);

    TouchpadSettings settings;

    settings.click_mode = mir_touchpad_click_mode_none;
    if (click_modes & LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS)
        settings.click_mode |= mir_touchpad_click_mode_area_to_click;
    if (click_modes & LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER)
        settings.click_mode |= mir_touchpad_click_mode_finger_count;

    switch (libinput_device_config_scroll_get_method(dev))
    {
    case LIBINPUT_CONFIG_SCROLL_NO_SCROLL:
        settings.scroll_mode = mir_touchpad_scroll_mode_none;
        break;
    case LIBINPUT_CONFIG_SCROLL_2FG:
        settings.scroll_mode = mir_touchpad_scroll_mode_two_finger_scroll;
        break;
    case LIBINPUT_CONFIG_SCROLL_EDGE:
        settings.scroll_mode = mir_touchpad_scroll_mode_edge_scroll;
        break;
    case LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN:
        settings.scroll_mode = mir_touchpad_scroll_mode_button_down_scroll;
        break;
    }

    settings.tap_to_click = libinput_device_config_tap_get_enabled(dev) == LIBINPUT_CONFIG_TAP_ENABLED;
    settings.disable_while_typing = libinput_device_config_dwt_get_enabled(dev) == LIBINPUT_CONFIG_DWT_ENABLED;
    settings.disable_with_mouse =
        libinput_device_config_send_events_get_mode(dev) == LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE;
    settings.middle_mouse_button_emulation =
        libinput_device_config_middle_emulation_get_enabled(dev) == LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED;

    return settings;
}

namespace
{
void apply_scroll_mode(libinput_device* dev, MirTouchpadScrollModes scroll_mode)
{
    auto set_method = [dev](libinput_config_scroll_method method)
    {
        if (LIBINPUT_CONFIG_STATUS_SUCCESS != libinput_device_config_scroll_set_method(dev, method))
        {
            auto const default_method = libinput_device_config_scroll_get_default_method(dev);
            mir::log_info("On device '%s': Failed to set scroll method to %d, using default (%d)", libinput_device_get_name(dev), method, default_method);
            libinput_device_config_scroll_set_method(dev, default_method);
        }
    };
    switch (scroll_mode)
    {
    case mir_touchpad_scroll_mode_none:
        set_method(LIBINPUT_CONFIG_SCROLL_NO_SCROLL);
        break;

    case mir_touchpad_scroll_mode_button_down_scroll:
    {
        set_method(LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN);
        break;
    }
    case mir_touchpad_scroll_mode_edge_scroll:
        set_method(LIBINPUT_CONFIG_SCROLL_EDGE);
        break;

    case mir_touchpad_scroll_mode_two_finger_scroll:
        set_method(LIBINPUT_CONFIG_SCROLL_2FG);
        break;
    }
}
}

void mie::LibInputDevice::apply_settings(mi::TouchpadSettings const& settings)
{
    auto dev = device();

    apply_scroll_mode(dev, settings.scroll_mode);
    if (const auto scroll_button = settings.button_down_scroll_button; scroll_button != no_scroll_button)
    {
        libinput_device_config_scroll_set_button(dev, scroll_button);
    }

    uint32_t click_method = LIBINPUT_CONFIG_CLICK_METHOD_NONE;
    if (settings.click_mode & mir_touchpad_click_mode_area_to_click)
        click_method |= LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;
    if (settings.click_mode & mir_touchpad_click_mode_finger_count)
        click_method |= LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER;

    libinput_device_config_click_set_method(dev, static_cast<libinput_config_click_method>(click_method));

    libinput_device_config_tap_set_enabled(
        dev, settings.tap_to_click ? LIBINPUT_CONFIG_TAP_ENABLED : LIBINPUT_CONFIG_TAP_DISABLED);

    libinput_device_config_dwt_set_enabled(
        dev, settings.disable_while_typing ? LIBINPUT_CONFIG_DWT_ENABLED : LIBINPUT_CONFIG_DWT_DISABLED);

    libinput_device_config_send_events_set_mode(dev, settings.disable_with_mouse ?
                                                         LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE :
                                                         LIBINPUT_CONFIG_SEND_EVENTS_ENABLED);

    libinput_device_config_middle_emulation_set_enabled(dev, settings.middle_mouse_button_emulation ?
                                                                 LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED :
                                                                 LIBINPUT_CONFIG_MIDDLE_EMULATION_DISABLED);
}

mir::optional_value<mi::TouchscreenSettings> mie::LibInputDevice::get_touchscreen_settings() const
{
    return touchscreen;
}

void mie::LibInputDevice::apply_settings(mi::TouchscreenSettings const& settings)
{
    touchscreen = settings;
}
