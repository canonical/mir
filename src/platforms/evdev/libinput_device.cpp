/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "libinput_device.h"
#include "libinput_ptr.h"
#include "libinput_device_ptr.h"
#include "evdev_device_detection.h"
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
#include <dlfcn.h>

#include <boost/exception/diagnostic_information.hpp>
#include <cstring>
#include <chrono>
#include <sstream>
#include <algorithm>

namespace md = mir::dispatch;
namespace mi = mir::input;
namespace mie = mi::evdev;
using namespace std::literals::chrono_literals;

namespace
{
double touch_major(libinput_event_touch*, uint32_t, uint32_t)
{
    return 8;
}
double touch_minor(libinput_event_touch*, uint32_t, uint32_t)
{
    return 6;
}
double pressure(libinput_event_touch*)
{
    return 0.8;
}
double orientation(libinput_event_touch*)
{
    return 0;
}

template<typename T> auto load_function(char const* sym)
{
    T result{};
    dlerror();
    (void*&)result = dlsym(RTLD_DEFAULT, sym);
    char const *error = dlerror();

    if (error)
        throw std::runtime_error(error);
    if (!result)
        throw std::runtime_error("no valid function address");

    return result;
}
}

struct mie::LibInputDevice::ContactExtension
{
    ContactExtension()
    {
        constexpr const char touch_major_sym[] = "libinput_event_touch_get_major_transformed";
        constexpr const char touch_minor_sym[] = "libinput_event_touch_get_minor_transformed";
        constexpr const char orientation_sym[] = "libinput_event_touch_get_orientation";
        constexpr const char pressure_sym[] = "libinput_event_touch_get_pressure";

        try
        {
            get_touch_major = load_function<decltype(&touch_major)>(touch_major_sym);
            get_touch_minor = load_function<decltype(&touch_minor)>(touch_minor_sym);
            get_orientation = load_function<decltype(&orientation)>(orientation_sym);
            get_pressure = load_function<decltype(&pressure)>(pressure_sym);
        }
        catch(...) {}
    }

    decltype(&touch_major) get_touch_major{&touch_major};
    decltype(&touch_minor) get_touch_minor{&touch_minor};
    decltype(&pressure) get_pressure{&pressure};
    decltype(&orientation) get_orientation{&orientation};
};

mie::LibInputDevice::LibInputDevice(std::shared_ptr<mi::InputReport> const& report, LibInputDevicePtr dev)
    : contact_extension{std::make_unique<ContactExtension>()}, report{report}, pointer_pos{0, 0}, button_state{0}
{
    add_device_of_group(std::move(dev));
}

void mie::LibInputDevice::add_device_of_group(LibInputDevicePtr dev)
{
    devices.emplace_back(std::move(dev));
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
        case LIBINPUT_EVENT_POINTER_AXIS:
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
                sink->handle_input(convert_touch_frame(libinput_event_get_touch_event(event)));
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
    auto const relative_x_value = 0.0f;
    auto const relative_y_value = 0.0f;
    auto const hscroll_value = 0.0f;
    auto const vscroll_value = 0.0f;

    report->received_event_from_kernel(time.count(), EV_KEY, pointer_button, action);

    if (action == mir_pointer_action_button_down)
        button_state = MirPointerButton(button_state | uint32_t(pointer_button));
    else
        button_state = MirPointerButton(button_state & ~uint32_t(pointer_button));

    return builder->pointer_event(time, action, button_state, hscroll_value, vscroll_value, relative_x_value, relative_y_value);
}

mir::EventUPtr mie::LibInputDevice::convert_motion_event(libinput_event_pointer* pointer)
{
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_pointer_get_time_usec(pointer));
    auto const action = mir_pointer_action_motion;
    auto const hscroll_value = 0.0f;
    auto const vscroll_value = 0.0f;

    report->received_event_from_kernel(time.count(), EV_REL, 0, 0);

    return builder->pointer_event(time, action, button_state,
                                  hscroll_value, vscroll_value,
                                  libinput_event_pointer_get_dx(pointer),
                                  libinput_event_pointer_get_dy(pointer));
}

mir::EventUPtr mie::LibInputDevice::convert_absolute_motion_event(libinput_event_pointer* pointer)
{
    // a pointing device that emits absolute coordinates
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_pointer_get_time_usec(pointer));
    auto const action = mir_pointer_action_motion;
    auto const hscroll_value = 0.0f;
    auto const vscroll_value = 0.0f;
    // either the bounding box .. or the specific output ..
    auto const screen = sink->bounding_rectangle();
    uint32_t const width = screen.size.width.as_int();
    uint32_t const height = screen.size.height.as_int();
    auto abs_x = libinput_event_pointer_get_absolute_x_transformed(pointer, width);
    auto abs_y = libinput_event_pointer_get_absolute_y_transformed(pointer, height);

    report->received_event_from_kernel(time.count(), EV_ABS, 0, 0);
    auto const old_pointer_pos = pointer_pos;
    pointer_pos = mir::geometry::Point{abs_x, abs_y};
    auto const movement = pointer_pos - old_pointer_pos;

    return builder->pointer_event(time, action, button_state, abs_x, abs_y, hscroll_value, vscroll_value,
                                  movement.dx.as_int(), movement.dy.as_int());
}

mir::EventUPtr mie::LibInputDevice::convert_axis_event(libinput_event_pointer* pointer)
{
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_pointer_get_time_usec(pointer));
    auto const action = mir_pointer_action_motion;
    auto const relative_x_value = 0.0f;
    auto const relative_y_value = 0.0f;

    auto hscroll_value = 0.0f;
    auto vscroll_value = 0.0f;
    if (libinput_event_pointer_get_axis_source(pointer) == LIBINPUT_POINTER_AXIS_SOURCE_WHEEL)
    {
        if (libinput_event_pointer_has_axis(pointer, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
            hscroll_value = horizontal_scroll_scale * libinput_event_pointer_get_axis_value_discrete(
                                                          pointer, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
        if (libinput_event_pointer_has_axis(pointer, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
            vscroll_value = -vertical_scroll_scale * libinput_event_pointer_get_axis_value_discrete(
                                                        pointer, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
    }
    else
    {
        // by default libinput assumes that wheel ticks represent a rotation of 15 degrees. This relation
        // is used to map wheel rotations to 'scroll units'. To map the immediate scroll units received
        // from gesture based scrolling we invert that transformation here.
        auto const scroll_units_to_ticks = 15.0f;
        if (libinput_event_pointer_has_axis(pointer, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
            hscroll_value = horizontal_scroll_scale *
                            libinput_event_pointer_get_axis_value(pointer, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL) /
                            scroll_units_to_ticks;

        if (libinput_event_pointer_has_axis(pointer, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
            vscroll_value = -vertical_scroll_scale *
                            libinput_event_pointer_get_axis_value(pointer, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL) /
                            scroll_units_to_ticks;
    }

    report->received_event_from_kernel(time.count(), EV_REL, 0, 0);
    return builder->pointer_event(time, action, button_state, hscroll_value, vscroll_value, relative_x_value,
                                  relative_y_value);
}

mir::EventUPtr mie::LibInputDevice::convert_touch_frame(libinput_event_touch* touch)
{
    std::chrono::nanoseconds const time = std::chrono::microseconds(libinput_event_touch_get_time_usec(touch));
    report->received_event_from_kernel(time.count(), EV_SYN, 0, 0);

    // TODO make libinput indicate tool type
    auto const tool = mir_touch_tooltype_finger;

    std::vector<events::ContactState> contacts;
    for(auto it = begin(last_seen_properties); it != end(last_seen_properties);)
    {
        auto & id = it->first;
        auto & data = it->second;

        contacts.push_back(events::ContactState{
                           id,
                           data.action,
                           tool,
                           data.x,
                           data.y,
                           data.pressure,
                           data.major,
                           data.minor,
                           data.orientation});

        if (data.action == mir_touch_action_down)
            data.action = mir_touch_action_change;

        if (data.action == mir_touch_action_up)
            it = last_seen_properties.erase(it);
        else
            ++it;
    }

    return builder->touch_event(time, contacts);
}

void mie::LibInputDevice::handle_touch_down(libinput_event_touch* touch)
{
    MirTouchId const id = libinput_event_touch_get_slot(touch);
    update_contact_data(last_seen_properties[id], mir_touch_action_down, touch);
}

void mie::LibInputDevice::handle_touch_up(libinput_event_touch* touch)
{
    MirTouchId const id = libinput_event_touch_get_slot(touch);
    last_seen_properties[id].action = mir_touch_action_up;
}

void mie::LibInputDevice::update_contact_data(ContactData & data, MirTouchAction action, libinput_event_touch* touch)
{
    auto info = get_output_info();

    uint32_t width = info.output_size.width.as_int();
    uint32_t height = info.output_size.height.as_int();

    data.action = action;
    data.x = libinput_event_touch_get_x_transformed(touch, width);
    data.y = libinput_event_touch_get_y_transformed(touch, height);
    data.major = contact_extension->get_touch_major(touch, width, height);
    data.minor = contact_extension->get_touch_minor(touch, width, height);
    data.pressure = contact_extension->get_pressure(touch);

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

    for (auto const& dev : devices)
    {
        auto const u_dev = mir::raii::deleter_for(libinput_device_get_udev_device(dev.get()), &udev_device_unref);
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
    }

    if (contains(caps, mi::DeviceCapability::touchscreen) &&
        !contains(info.capabilities, mi::DeviceCapability::touchscreen))
    {
        touchscreen = mi::TouchscreenSettings{};

        // FIXME: hack to connect touchscreen to output. {arg}
        touchscreen.value().output_id = 1;
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
    return devices.front().get();
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
#if MIR_LIBINPUT_HAS_ACCEL_PROFILE
    if (libinput_device_config_accel_get_profile(dev) == LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT)
#endif
        settings.acceleration = mir_pointer_acceleration_none;
#if MIR_LIBINPUT_HAS_ACCEL_PROFILE
    else
        settings.acceleration = mir_pointer_acceleration_adaptive;
#endif
    settings.cursor_acceleration_bias = libinput_device_config_accel_get_speed(dev);
    settings.vertical_scroll_scale = vertical_scroll_scale;
    settings.horizontal_scroll_scale = horizontal_scroll_scale;
    return settings;
}

void mie::LibInputDevice::apply_settings(mir::input::PointerSettings const& settings)
{
    if (!contains(info.capabilities, mi::DeviceCapability::pointer))
        return;

    auto dev = device();

#if MIR_LIBINPUT_HAS_ACCEL_PROFILE
    auto accel_profile = settings.acceleration == mir_pointer_acceleration_adaptive ?
        LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE :
        LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
#endif
    libinput_device_config_accel_set_speed(dev, settings.cursor_acceleration_bias);
    libinput_device_config_left_handed_set(dev, mir_pointer_handedness_left == settings.handedness);
    vertical_scroll_scale = settings.vertical_scroll_scale;
    horizontal_scroll_scale = settings.horizontal_scroll_scale;
#if MIR_LIBINPUT_HAS_ACCEL_PROFILE
    libinput_device_config_accel_set_profile(dev, accel_profile);
#endif
}

mir::optional_value<mi::TouchpadSettings> mie::LibInputDevice::get_touchpad_settings() const
{
    if (!contains(info.capabilities, mi::DeviceCapability::touchpad))
        return {};

    auto dev = device();
    auto click_modes = libinput_device_config_click_get_method(dev);
    auto scroll_modes = libinput_device_config_scroll_get_method(dev);

    TouchpadSettings settings;

    settings.click_mode = mir_touchpad_click_mode_none;
    if (click_modes & LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS)
        settings.click_mode |= mir_touchpad_click_mode_area_to_click;
    if (click_modes & LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER)
        settings.click_mode |= mir_touchpad_click_mode_finger_count;

    settings.scroll_mode = mir_touchpad_scroll_mode_none;
    if (scroll_modes & LIBINPUT_CONFIG_SCROLL_2FG)
        settings.scroll_mode |= mir_touchpad_scroll_mode_two_finger_scroll;
    if (scroll_modes & LIBINPUT_CONFIG_SCROLL_EDGE)
        settings.scroll_mode |= mir_touchpad_scroll_mode_edge_scroll;
    if (scroll_modes & LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN)
        settings.scroll_mode |= mir_touchpad_scroll_mode_button_down_scroll;

    settings.tap_to_click = libinput_device_config_tap_get_enabled(dev) == LIBINPUT_CONFIG_TAP_ENABLED;
    settings.disable_while_typing = libinput_device_config_dwt_get_enabled(dev) == LIBINPUT_CONFIG_DWT_ENABLED;
    settings.disable_with_mouse =
        libinput_device_config_send_events_get_mode(dev) == LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE;
    settings.middle_mouse_button_emulation =
        libinput_device_config_middle_emulation_get_enabled(dev) == LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED;

    return settings;
}

void mie::LibInputDevice::apply_settings(mi::TouchpadSettings const& settings)
{
    auto dev = device();

    uint32_t click_method = LIBINPUT_CONFIG_CLICK_METHOD_NONE;
    if (settings.click_mode & mir_touchpad_click_mode_area_to_click)
        click_method |= LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;
    if (settings.click_mode & mir_touchpad_click_mode_finger_count)
        click_method |= LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER;

    uint32_t scroll_method = LIBINPUT_CONFIG_CLICK_METHOD_NONE;
    if (settings.scroll_mode & mir_touchpad_scroll_mode_button_down_scroll)
    {
        scroll_method |= LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN;
        libinput_device_config_scroll_set_button(dev, settings.button_down_scroll_button);
    }
    if (settings.scroll_mode & mir_touchpad_scroll_mode_edge_scroll)
        scroll_method |= LIBINPUT_CONFIG_SCROLL_EDGE;
    if (settings.scroll_mode & mir_touchpad_scroll_mode_two_finger_scroll)
        scroll_method |= LIBINPUT_CONFIG_SCROLL_2FG;

    libinput_device_config_click_set_method(dev, static_cast<libinput_config_click_method>(click_method));
    libinput_device_config_scroll_set_method(dev, static_cast<libinput_config_scroll_method>(scroll_method));

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
