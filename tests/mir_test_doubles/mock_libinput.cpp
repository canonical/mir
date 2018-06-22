/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/test/doubles/mock_libinput.h"

namespace mtd = mir::test::doubles;
using namespace testing;

namespace
{
mtd::MockLibInput* global_libinput = nullptr;
}

mtd::MockLibInput::MockLibInput()
{
    assert(global_libinput == NULL && "Only one mock object per process is allowed");
    global_libinput = this;

    ON_CALL(*this, libinput_device_ref(_)).WillByDefault(ReturnArg<0>());
    ON_CALL(*this, libinput_get_fd(_)).WillByDefault(Return(int(libinput_simulation_queue.watch_fd())));
    ON_CALL(*this, libinput_get_event(_))
        .WillByDefault(Invoke([this](libinput*) -> libinput_event*
                              {
                                  if (events.empty())
                                      return nullptr;
                                  libinput_simulation_queue.dispatch(mir::dispatch::FdEvent::readable);
                                  auto ret = events.front();
                                  events.erase(events.begin());
                                  return ret;
                              }
                             ));
    ON_CALL(*this, libinput_device_config_left_handed_set(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
    ON_CALL(*this, libinput_device_config_accel_set_speed(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
    ON_CALL(*this, libinput_device_config_dwt_set_enabled(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
    ON_CALL(*this, libinput_device_config_scroll_set_button(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
    ON_CALL(*this, libinput_device_config_scroll_set_method(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
    ON_CALL(*this, libinput_device_config_click_set_method(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
    ON_CALL(*this, libinput_device_config_tap_set_enabled(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
    ON_CALL(*this, libinput_device_config_send_events_set_mode(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
    ON_CALL(*this, libinput_device_config_middle_emulation_set_enabled(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
#if MIR_LIBINPUT_HAS_ACCEL_PROFILE
    ON_CALL(*this, libinput_device_config_accel_set_profile(_, _))
        .WillByDefault(Return(LIBINPUT_CONFIG_STATUS_SUCCESS));
#endif
}

void mtd::MockLibInput::wake()
{
    libinput_simulation_queue.enqueue([]{});
}

void mtd::MockLibInput::push_back(libinput_event* event)
{
    events.push_back(event);
    wake();
}

void mtd::MockLibInput::setup_device(libinput_device* dev, libinput_device_group* group, std::shared_ptr<udev_device> u_dev, char const* name, char const* sysname, unsigned int vendor, unsigned int product)
{
    ON_CALL(*this, libinput_device_get_name(dev))
        .WillByDefault(Return(name));
    ON_CALL(*this, libinput_device_get_sysname(dev))
        .WillByDefault(Return(sysname));
    ON_CALL(*this, libinput_device_get_device_group(dev))
        .WillByDefault(Return(group));
    ON_CALL(*this, libinput_device_get_id_product(dev))
        .WillByDefault(Return(product));
    ON_CALL(*this, libinput_device_ref(dev))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_device_get_id_vendor(dev))
        .WillByDefault(Return(vendor));
    ON_CALL(*this, libinput_device_unref(dev))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, libinput_device_get_udev_device(dev))
        .WillByDefault(InvokeWithoutArgs([u_dev]() { return udev_device_ref(u_dev.get());}));
}

mtd::MockLibInput::~MockLibInput() noexcept
{
    global_libinput = nullptr;
}

void libinput_event_destroy(libinput_event* event)
{
    global_libinput->libinput_event_destroy(event);
}

libinput_event_type libinput_event_get_type(libinput_event* event)
{
    return global_libinput->libinput_event_get_type(event);
}

libinput_device* libinput_event_get_device(libinput_event* event)
{
    return global_libinput->libinput_event_get_device(event);
}

libinput_event_pointer* libinput_event_get_pointer_event(libinput_event* event)
{
    return global_libinput->libinput_event_get_pointer_event(event);
}

libinput_event_keyboard* libinput_event_get_keyboard_event(libinput_event* event)
{
    return global_libinput->libinput_event_get_keyboard_event(event);
}

libinput_event_touch* libinput_event_get_touch_event(libinput_event* event)
{
    return global_libinput->libinput_event_get_touch_event(event);
}

uint32_t libinput_event_keyboard_get_time(libinput_event_keyboard* event)
{
    return global_libinput->libinput_event_keyboard_get_time(event);
}

uint64_t libinput_event_keyboard_get_time_usec(libinput_event_keyboard* event)
{
    return global_libinput->libinput_event_keyboard_get_time_usec(event);
}

uint32_t libinput_event_keyboard_get_key(libinput_event_keyboard* event)
{
    return global_libinput->libinput_event_keyboard_get_key(event);
}

libinput_key_state libinput_event_keyboard_get_key_state(libinput_event_keyboard* event)
{
    return global_libinput->libinput_event_keyboard_get_key_state(event);
}

uint32_t libinput_event_keyboard_get_seat_key_count(libinput_event_keyboard* event)
{
    return global_libinput->libinput_event_keyboard_get_seat_key_count(event);
}

uint32_t libinput_event_pointer_get_time(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_time(event);
}

uint64_t libinput_event_pointer_get_time_usec(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_time_usec(event);
}

double libinput_event_pointer_get_dx(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_dx(event);
}

double libinput_event_pointer_get_dy(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_dy(event);
}

double libinput_event_pointer_get_absolute_x(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_absolute_x(event);
}

double libinput_event_pointer_get_absolute_y(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_absolute_y(event);
}

double libinput_event_pointer_get_absolute_x_transformed(libinput_event_pointer* event, uint32_t width)
{
    return global_libinput->libinput_event_pointer_get_absolute_x_transformed(event, width);
}

double libinput_event_pointer_get_absolute_y_transformed(libinput_event_pointer* event, uint32_t height)
{
    return global_libinput->libinput_event_pointer_get_absolute_y_transformed(event, height);
}

uint32_t libinput_event_pointer_get_button(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_button(event);
}

libinput_button_state libinput_event_pointer_get_button_state(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_button_state(event);
}

uint32_t libinput_event_pointer_get_seat_button_count(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_seat_button_count(event);
}

libinput_pointer_axis libinput_event_pointer_get_axis(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_axis(event);
}

libinput_pointer_axis_source libinput_event_pointer_get_axis_source(libinput_event_pointer* event)
{
    return global_libinput->libinput_event_pointer_get_axis_source(event);
}

double libinput_event_pointer_get_axis_value(libinput_event_pointer* event, libinput_pointer_axis axis)
{
    return global_libinput->libinput_event_pointer_get_axis_value(event, axis);
}

double libinput_event_pointer_get_axis_value_discrete(libinput_event_pointer* event, libinput_pointer_axis axis)
{
    return global_libinput->libinput_event_pointer_get_axis_value_discrete(event, axis);
}

int libinput_event_pointer_has_axis(libinput_event_pointer* event, libinput_pointer_axis axis)
{
    return global_libinput->libinput_event_pointer_has_axis(event, axis);
}

uint32_t libinput_event_touch_get_time(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_time(event);
}

uint64_t libinput_event_touch_get_time_usec(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_time_usec(event);
}

int32_t libinput_event_touch_get_slot(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_slot(event);
}

int32_t libinput_event_touch_get_seat_slot(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_seat_slot(event);
}

double libinput_event_touch_get_x(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_x(event);
}

double libinput_event_touch_get_y(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_y(event);
}

double libinput_event_touch_get_x_transformed(libinput_event_touch* event, uint32_t width)
{
    return global_libinput->libinput_event_touch_get_x_transformed(event, width);
}

double libinput_event_touch_get_y_transformed(libinput_event_touch* event, uint32_t height)
{
    return global_libinput->libinput_event_touch_get_y_transformed(event, height);
}

extern "C" double libinput_event_touch_get_major(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_major(event);
}

extern "C" double libinput_event_touch_get_minor(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_minor(event);
}

extern "C" double libinput_event_touch_get_major_transformed(libinput_event_touch* event, uint32_t width, uint32_t height)
{
    return global_libinput->libinput_event_touch_get_major_transformed(event, width, height);
}

extern "C" double libinput_event_touch_get_minor_transformed(libinput_event_touch* event, uint32_t width, uint32_t height)
{
    return global_libinput->libinput_event_touch_get_minor_transformed(event, width, height);
}

extern "C" double libinput_event_touch_get_pressure(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_pressure(event);
}

extern "C" double libinput_event_touch_get_orientation(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_orientation(event);
}

libinput* libinput_udev_create_context(const libinput_interface* interface, void* user_data, struct udev* udev)
{
    return global_libinput->libinput_udev_create_context(interface, user_data, udev);
}

int libinput_udev_assign_seat(libinput* ctx, char const* seat)
{
    return global_libinput->libinput_udev_assign_seat(ctx, seat);
}

libinput* libinput_path_create_context(const libinput_interface* interface, void* user_data)
{
    return global_libinput->libinput_path_create_context(interface, user_data);
}

libinput_device* libinput_path_add_device(libinput* libinput, const char* path)
{
    return global_libinput->libinput_path_add_device(libinput, path);
}

void libinput_path_remove_device(libinput_device* device)
{
    return global_libinput->libinput_path_remove_device(device);
}

int libinput_get_fd(libinput* libinput)
{
    return global_libinput->libinput_get_fd(libinput);
}

int libinput_dispatch(libinput* libinput)
{
    return global_libinput->libinput_dispatch(libinput);
}

libinput_event* libinput_get_event(libinput* libinput)
{
    return global_libinput->libinput_get_event(libinput);
}

libinput* libinput_ref(libinput* libinput)
{
    return global_libinput->libinput_ref(libinput);
}

libinput* libinput_unref(libinput* libinput)
{
    return global_libinput->libinput_unref(libinput);
}

libinput_device* libinput_device_ref(libinput_device* device)
{
    return global_libinput->libinput_device_ref(device);
}

libinput_device* libinput_device_unref(libinput_device* device)
{
    return global_libinput->libinput_device_unref(device);
}

char const* libinput_device_get_name(libinput_device* device)
{
    return global_libinput->libinput_device_get_name(device);
}

udev_device* libinput_device_get_udev_device(libinput_device* device)
{
    return global_libinput->libinput_device_get_udev_device(device);
}

unsigned int libinput_device_get_id_vendor(libinput_device* device)
{
    return global_libinput->libinput_device_get_id_vendor(device);
}

unsigned int libinput_device_get_id_product(libinput_device* device)
{
    return global_libinput->libinput_device_get_id_product(device);
}

char const* libinput_device_get_sysname(libinput_device* device)
{
    return global_libinput->libinput_device_get_sysname(device);
}

libinput_device_group* libinput_device_get_device_group(libinput_device* device)
{
    return global_libinput->libinput_device_get_device_group(device);
}

int libinput_device_config_tap_get_finger_count(libinput_device *device)
{
    return global_libinput->libinput_device_config_tap_get_finger_count(device);
}

libinput_config_status libinput_device_config_tap_set_enabled(libinput_device *device, libinput_config_tap_state enable)
{
    return global_libinput->libinput_device_config_tap_set_enabled(device, enable);
}

libinput_config_tap_state libinput_device_config_tap_get_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_tap_get_enabled(device);
}


libinput_config_tap_state libinput_device_config_tap_get_default_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_tap_get_default_enabled(device);
}

libinput_config_status libinput_device_config_tap_set_drag_lock_enabled(libinput_device *device, libinput_config_drag_lock_state enable)
{
    return global_libinput->libinput_device_config_tap_set_drag_lock_enabled(device, enable);
}

libinput_config_drag_lock_state libinput_device_config_tap_get_drag_lock_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_tap_get_drag_lock_enabled(device);
}

libinput_config_drag_lock_state libinput_device_config_tap_get_default_drag_lock_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_tap_get_default_drag_lock_enabled(device);
}

int libinput_device_config_calibration_has_matrix(libinput_device *device)
{
    return global_libinput->libinput_device_config_calibration_has_matrix(device);
}

libinput_config_status libinput_device_config_calibration_set_matrix(libinput_device *device, const float matrix[6])
{
    return global_libinput->libinput_device_config_calibration_set_matrix(device, matrix);
}

int libinput_device_config_calibration_get_matrix(libinput_device *device, float matrix[6])
{
    return global_libinput->libinput_device_config_calibration_get_matrix(device, matrix);
}

int libinput_device_config_calibration_get_default_matrix(libinput_device *device, float matrix[6])
{
    return global_libinput->libinput_device_config_calibration_get_default_matrix(device, matrix);
}

uint32_t libinput_device_config_send_events_get_modes(libinput_device *device)
{
    return global_libinput->libinput_device_config_send_events_get_modes(device);
}

libinput_config_status libinput_device_config_send_events_set_mode(libinput_device *device, uint32_t mode)
{
    return global_libinput->libinput_device_config_send_events_set_mode(device, mode);
}

uint32_t libinput_device_config_send_events_get_mode(libinput_device *device)
{
    return global_libinput->libinput_device_config_send_events_get_mode(device);
}

uint32_t libinput_device_config_send_events_get_default_mode(libinput_device *device)
{
    return global_libinput->libinput_device_config_send_events_get_default_mode(device);
}

int libinput_device_config_accel_is_available(libinput_device *device)
{
    return global_libinput->libinput_device_config_accel_is_available(device);
}

libinput_config_status libinput_device_config_accel_set_speed(libinput_device *device, double speed)
{
    return global_libinput->libinput_device_config_accel_set_speed(device, speed);
}

double libinput_device_config_accel_get_speed(libinput_device *device)
{
    return global_libinput->libinput_device_config_accel_get_speed(device);
}

double libinput_device_config_accel_get_default_speed(libinput_device *device)
{
    return global_libinput->libinput_device_config_accel_get_default_speed(device);
}

#if MIR_LIBINPUT_HAS_ACCEL_PROFILE
libinput_config_status libinput_device_config_accel_set_profile(libinput_device* dev, libinput_config_accel_profile profile)
{
    return global_libinput->libinput_device_config_accel_set_profile(dev, profile);
}

libinput_config_accel_profile libinput_device_config_accel_get_profile(libinput_device* dev)
{
    return global_libinput->libinput_device_config_accel_get_profile(dev);
}
#endif

int libinput_device_config_scroll_has_natural_scroll(libinput_device *device)
{
    return global_libinput->libinput_device_config_scroll_has_natural_scroll(device);
}

libinput_config_status libinput_device_config_scroll_set_natural_scroll_enabled(libinput_device *device, int enable)
{
    return global_libinput->libinput_device_config_scroll_set_natural_scroll_enabled(device, enable);
}

int libinput_device_config_scroll_get_natural_scroll_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_scroll_get_natural_scroll_enabled(device);
}

int libinput_device_config_scroll_get_default_natural_scroll_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_scroll_get_default_natural_scroll_enabled(device);
}

int libinput_device_config_left_handed_is_available(libinput_device *device)
{
    return global_libinput->libinput_device_config_left_handed_is_available(device);
}

libinput_config_status libinput_device_config_left_handed_set(libinput_device *device, int left_handed)
{
    return global_libinput->libinput_device_config_left_handed_set(device, left_handed);
}

int libinput_device_config_left_handed_get(libinput_device *device)
{
    return global_libinput->libinput_device_config_left_handed_get(device);
}

int libinput_device_config_left_handed_get_default(libinput_device *device)
{
    return global_libinput->libinput_device_config_left_handed_get_default(device);
}

uint32_t libinput_device_config_click_get_methods(libinput_device *device)
{
    return global_libinput->libinput_device_config_click_get_methods(device);
}

libinput_config_status libinput_device_config_click_set_method(libinput_device *device, libinput_config_click_method method)
{
    return global_libinput->libinput_device_config_click_set_method(device, method);
}

libinput_config_click_method libinput_device_config_click_get_method(libinput_device *device)
{
    return global_libinput->libinput_device_config_click_get_method(device);
}

libinput_config_click_method libinput_device_config_click_get_default_method(libinput_device *device)
{
    return global_libinput->libinput_device_config_click_get_default_method(device);
}

int libinput_device_config_middle_emulation_is_available(libinput_device *device)
{
    return global_libinput->libinput_device_config_middle_emulation_is_available(device);
}

libinput_config_status libinput_device_config_middle_emulation_set_enabled(libinput_device *device, libinput_config_middle_emulation_state enable)
{
    return global_libinput->libinput_device_config_middle_emulation_set_enabled(device, enable);
}

libinput_config_middle_emulation_state libinput_device_config_middle_emulation_get_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_middle_emulation_get_enabled(device);
}

libinput_config_middle_emulation_state libinput_device_config_middle_emulation_get_default_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_middle_emulation_get_default_enabled(device);
}

uint32_t libinput_device_config_scroll_get_methods(libinput_device *device)
{
    return global_libinput->libinput_device_config_scroll_get_methods(device);
}

libinput_config_status libinput_device_config_scroll_set_method(libinput_device *device, libinput_config_scroll_method method)
{
    return global_libinput->libinput_device_config_scroll_set_method(device, method);
}

libinput_config_scroll_method libinput_device_config_scroll_get_method(libinput_device *device)
{
    return global_libinput->libinput_device_config_scroll_get_method(device);
}

libinput_config_scroll_method libinput_device_config_scroll_get_default_method(libinput_device *device)
{
    return global_libinput->libinput_device_config_scroll_get_default_method(device);
}

libinput_config_status libinput_device_config_scroll_set_button(libinput_device *device, uint32_t button)
{
    return global_libinput->libinput_device_config_scroll_set_button(device, button);
}

uint32_t libinput_device_config_scroll_get_button(libinput_device *device)
{
    return global_libinput->libinput_device_config_scroll_get_button(device);
}

uint32_t libinput_device_config_scroll_get_default_button(libinput_device *device)
{
    return global_libinput->libinput_device_config_scroll_get_default_button(device);
}

int libinput_device_config_dwt_is_available(libinput_device *device)
{
    return global_libinput->libinput_device_config_dwt_is_available(device);
}

libinput_config_status libinput_device_config_dwt_set_enabled(libinput_device *device, libinput_config_dwt_state enable)
{
    return global_libinput->libinput_device_config_dwt_set_enabled(device, enable);
}

libinput_config_dwt_state libinput_device_config_dwt_get_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_dwt_get_enabled(device);
}

libinput_config_dwt_state libinput_device_config_dwt_get_default_enabled(libinput_device *device)
{
    return global_libinput->libinput_device_config_dwt_get_default_enabled(device);
}

libinput_event* mtd::MockLibInput::setup_touch_event(libinput_device* dev, libinput_event_type type, uint64_t event_time, int slot,
                                                     float x, float y, float major, float minor, float pressure, float orientation)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto touch_event = reinterpret_cast<libinput_event_touch*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(type));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_get_touch_event(event))
        .WillByDefault(Return(touch_event));
    ON_CALL(*this, libinput_event_touch_get_slot(touch_event))
        .WillByDefault(Return(slot));
    ON_CALL(*this, libinput_event_touch_get_x_transformed(touch_event, _))
        .WillByDefault(Return(x));
    ON_CALL(*this, libinput_event_touch_get_y_transformed(touch_event, _))
        .WillByDefault(Return(y));
    ON_CALL(*this, libinput_event_touch_get_time_usec(touch_event))
        .WillByDefault(Return(event_time));
    ON_CALL(*this, libinput_event_touch_get_major_transformed(touch_event, _, _))
        .WillByDefault(Return(major));
    ON_CALL(*this, libinput_event_touch_get_minor_transformed(touch_event, _, _))
        .WillByDefault(Return(minor));
    ON_CALL(*this, libinput_event_touch_get_pressure(touch_event))
        .WillByDefault(Return(pressure));
    ON_CALL(*this, libinput_event_touch_get_orientation(touch_event))
        .WillByDefault(Return(orientation));

    return event;
}


libinput_event* mtd::MockLibInput::setup_touch_frame(libinput_device *dev, uint64_t event_time)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto touch_event = reinterpret_cast<libinput_event_touch*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_TOUCH_FRAME));
    ON_CALL(*this, libinput_event_get_touch_event(event))
        .WillByDefault(Return(touch_event));
    ON_CALL(*this, libinput_event_touch_get_time_usec(touch_event))
        .WillByDefault(Return(event_time));

    return event;
}

libinput_event* mtd::MockLibInput::setup_touch_up_event(libinput_device* dev, uint64_t event_time, int slot)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto touch_event = reinterpret_cast<libinput_event_touch*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_TOUCH_UP));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_get_touch_event(event))
        .WillByDefault(Return(touch_event));
    ON_CALL(*this, libinput_event_touch_get_slot(touch_event))
        .WillByDefault(Return(slot));
    ON_CALL(*this, libinput_event_touch_get_time_usec(touch_event))
        .WillByDefault(Return(event_time));

    return event;
}

libinput_event* mtd::MockLibInput::setup_key_event(libinput_device* dev, uint64_t event_time, uint32_t key, libinput_key_state state)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto key_event = reinterpret_cast<libinput_event_keyboard*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_KEYBOARD_KEY));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_get_keyboard_event(event))
        .WillByDefault(Return(key_event));
    ON_CALL(*this, libinput_event_keyboard_get_time_usec(key_event))
        .WillByDefault(Return(event_time));
    ON_CALL(*this, libinput_event_keyboard_get_key(key_event))
        .WillByDefault(Return(key));
    ON_CALL(*this, libinput_event_keyboard_get_key_state(key_event))
        .WillByDefault(Return(state));

    return event;
}

libinput_event* mtd::MockLibInput::setup_pointer_event(libinput_device* dev, uint64_t event_time, float relatve_x, float relatve_y)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_POINTER_MOTION));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_get_pointer_event(event))
        .WillByDefault(Return(pointer_event));
    ON_CALL(*this, libinput_event_pointer_get_time_usec(pointer_event))
        .WillByDefault(Return(event_time));
    ON_CALL(*this, libinput_event_pointer_get_dx(pointer_event))
        .WillByDefault(Return(relatve_x));
    ON_CALL(*this, libinput_event_pointer_get_dy(pointer_event))
        .WillByDefault(Return(relatve_y));
    return event;
}

libinput_event* mtd::MockLibInput::setup_absolute_pointer_event(libinput_device* dev, uint64_t event_time, float x, float y)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_get_pointer_event(event))
        .WillByDefault(Return(pointer_event));
    ON_CALL(*this, libinput_event_pointer_get_time_usec(pointer_event))
        .WillByDefault(Return(event_time));
    ON_CALL(*this, libinput_event_pointer_get_absolute_x_transformed(pointer_event, _))
        .WillByDefault(Return(x));
    ON_CALL(*this, libinput_event_pointer_get_absolute_y_transformed(pointer_event, _))
        .WillByDefault(Return(y));
    return event;
}

libinput_event* mtd::MockLibInput::setup_button_event(libinput_device* dev, uint64_t event_time, int button, libinput_button_state state)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_POINTER_BUTTON));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_get_pointer_event(event))
        .WillByDefault(Return(pointer_event));
    ON_CALL(*this, libinput_event_pointer_get_time_usec(pointer_event))
        .WillByDefault(Return(event_time));
    ON_CALL(*this, libinput_event_pointer_get_button(pointer_event))
        .WillByDefault(Return(button));
    ON_CALL(*this, libinput_event_pointer_get_button_state(pointer_event))
        .WillByDefault(Return(state));
    return event;
}

libinput_event* mtd::MockLibInput::setup_axis_event(libinput_device* dev, uint64_t event_time, double horizontal, double vertical)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_POINTER_AXIS));
    ON_CALL(*this, libinput_event_get_pointer_event(event))
        .WillByDefault(Return(pointer_event));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_pointer_get_time_usec(pointer_event))
        .WillByDefault(Return(event_time));
    ON_CALL(*this, libinput_event_pointer_get_axis_source(pointer_event))
        .WillByDefault(Return(LIBINPUT_POINTER_AXIS_SOURCE_WHEEL));
    ON_CALL(*this, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
        .WillByDefault(Return(horizontal!=0.0));
    ON_CALL(*this, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
        .WillByDefault(Return(vertical!=0.0));
    ON_CALL(*this, libinput_event_pointer_get_axis_value_discrete(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
        .WillByDefault(Return(vertical));
    ON_CALL(*this, libinput_event_pointer_get_axis_value_discrete(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
        .WillByDefault(Return(horizontal));
    return event;
}

libinput_event* mtd::MockLibInput::setup_finger_axis_event(libinput_device* dev, uint64_t event_time, double horizontal, double vertical)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_POINTER_AXIS));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    ON_CALL(*this, libinput_event_get_pointer_event(event))
        .WillByDefault(Return(pointer_event));
    ON_CALL(*this, libinput_event_pointer_get_time_usec(pointer_event))
        .WillByDefault(Return(event_time));
    ON_CALL(*this, libinput_event_pointer_get_axis_source(pointer_event))
        .WillByDefault(Return(LIBINPUT_POINTER_AXIS_SOURCE_FINGER));
    ON_CALL(*this, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
        .WillByDefault(Return(horizontal!=0.0));
    ON_CALL(*this, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
        .WillByDefault(Return(vertical!=0.0));
    ON_CALL(*this, libinput_event_pointer_get_axis_value(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
        .WillByDefault(Return(vertical));
    ON_CALL(*this, libinput_event_pointer_get_axis_value(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
        .WillByDefault(Return(horizontal));
    return event;
}

libinput_event* mtd::MockLibInput::setup_device_add_event(libinput_device* dev)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_DEVICE_ADDED));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    return event;
}

libinput_event* mtd::MockLibInput::setup_device_remove_event(libinput_device* dev)
{
    auto event = get_next_fake_ptr<libinput_event*>();
    push_back(event);

    ON_CALL(*this, libinput_event_get_type(event))
        .WillByDefault(Return(LIBINPUT_EVENT_DEVICE_REMOVED));
    ON_CALL(*this, libinput_event_get_device(event))
        .WillByDefault(Return(dev));
    return event;

}
