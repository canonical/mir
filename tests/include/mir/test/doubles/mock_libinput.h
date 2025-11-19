/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_MOCK_LIBINPUT_H_
#define MIR_TEST_DOUBLES_MOCK_LIBINPUT_H_

#include "mir/dispatch/action_queue.h"

#include <optional>
#include <gmock/gmock.h>
#include <libinput.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockLibInput
{
public:
    MockLibInput();
    ~MockLibInput() noexcept;
    void wake();
    void setup_device(libinput_device* device, libinput_device_group* group, std::shared_ptr<udev_device> u_dev, char const* name,
                      char const* sysname, unsigned int vendor, unsigned int product);

    libinput_event* setup_touch_event(libinput_device* dev, libinput_event_type type, uint64_t event_time, int slot,
                                      float x, float y, float major, float minor, float pressure, float orientation);
    libinput_event* setup_touch_frame(libinput_device *dev, uint64_t event_time);
    libinput_event* setup_touch_up_event(libinput_device* dev, uint64_t event_time, int slot);
    libinput_event* setup_key_event(libinput_device* dev, uint64_t event_time, uint32_t key, libinput_key_state state);
    libinput_event* setup_pointer_event(libinput_device* dev, uint64_t event_time, float relatve_x, float relatve_y);
    libinput_event* setup_absolute_pointer_event(libinput_device* dev, uint64_t event_time, float x, float y);
    libinput_event* setup_button_event(libinput_device* dev, uint64_t event_time, int button, libinput_button_state state);
    libinput_event* setup_axis_event(
        libinput_device* dev, uint64_t event_time,
        std::optional<double> horizontal, std::optional<double> vertical,
        double horizontal_discrete = 0.0, double vertical_discrete = 0.0);
    libinput_event* setup_pointer_scroll_wheel_event(
        libinput_device* dev, uint64_t event_time,
        std::optional<double> horizontal, std::optional<double> vertical,
        double horizontal_value120, double vertical_value120);
    libinput_event* setup_finger_axis_event(
        libinput_device* dev, uint64_t event_time,
        std::optional<double> horizontal, std::optional<double> vertical);
    libinput_event* setup_device_add_event(libinput_device* dev);
    libinput_event* setup_device_remove_event(libinput_device* dev);

    MOCK_METHOD(libinput*, libinput_ref, (libinput*), ());
    MOCK_METHOD(libinput*, libinput_unref, (libinput*), ());
    MOCK_METHOD(int, libinput_dispatch, (libinput*), ());
    MOCK_METHOD(int, libinput_get_fd, (libinput*), ());
    MOCK_METHOD(libinput_event*, libinput_get_event, (libinput*), ());
    MOCK_METHOD(libinput_event_type, libinput_event_get_type, (libinput_event*), ());
    MOCK_METHOD(void, libinput_event_destroy, (libinput_event*), ());
    MOCK_METHOD(libinput_device*, libinput_event_get_device, (libinput_event*), ());
    MOCK_METHOD(libinput_event_pointer*, libinput_event_get_pointer_event, (libinput_event*), ());
    MOCK_METHOD(libinput_event_keyboard*, libinput_event_get_keyboard_event, (libinput_event*), ());
    MOCK_METHOD(libinput_event_touch*, libinput_event_get_touch_event, (libinput_event*), ());

    MOCK_METHOD(uint32_t, libinput_event_keyboard_get_time, (libinput_event_keyboard*), ());
    MOCK_METHOD(uint64_t, libinput_event_keyboard_get_time_usec, (libinput_event_keyboard*), ());
    MOCK_METHOD(uint32_t, libinput_event_keyboard_get_key, (libinput_event_keyboard*), ());
    MOCK_METHOD(libinput_key_state, libinput_event_keyboard_get_key_state, (libinput_event_keyboard*), ());
    MOCK_METHOD(uint32_t, libinput_event_keyboard_get_seat_key_count, (libinput_event_keyboard*), ());

    MOCK_METHOD(uint32_t, libinput_event_pointer_get_time, (libinput_event_pointer*), ());
    MOCK_METHOD(uint64_t, libinput_event_pointer_get_time_usec, (libinput_event_pointer*), ());
    MOCK_METHOD(double, libinput_event_pointer_get_dx, (libinput_event_pointer*), ());
    MOCK_METHOD(double, libinput_event_pointer_get_dy, (libinput_event_pointer*), ());
    MOCK_METHOD(double, libinput_event_pointer_get_absolute_x, (libinput_event_pointer*), ());
    MOCK_METHOD(double, libinput_event_pointer_get_absolute_y, (libinput_event_pointer*), ());
    MOCK_METHOD(double, libinput_event_pointer_get_absolute_x_transformed, (libinput_event_pointer*, uint32_t), ());
    MOCK_METHOD(double, libinput_event_pointer_get_absolute_y_transformed, (libinput_event_pointer*, uint32_t), ());
    MOCK_METHOD(uint32_t, libinput_event_pointer_get_button, (libinput_event_pointer*), ());
    MOCK_METHOD(libinput_button_state, libinput_event_pointer_get_button_state, (libinput_event_pointer*), ());
    MOCK_METHOD(uint32_t, libinput_event_pointer_get_seat_button_count, (libinput_event_pointer*), ());
    MOCK_METHOD(libinput_pointer_axis, libinput_event_pointer_get_axis, (libinput_event_pointer*), ());
    MOCK_METHOD(double, libinput_event_pointer_get_axis_value, (libinput_event_pointer*, libinput_pointer_axis), ());
    MOCK_METHOD(libinput_pointer_axis_source, libinput_event_pointer_get_axis_source, (libinput_event_pointer*), ());
    MOCK_METHOD(double, libinput_event_pointer_get_axis_value_discrete, (libinput_event_pointer*, libinput_pointer_axis), ());
    MOCK_METHOD(double, libinput_event_pointer_get_scroll_value_v120, (libinput_event_pointer*, libinput_pointer_axis), ());
    MOCK_METHOD(libinput_event*, libinput_event_pointer_get_base_event, (libinput_event_pointer* event));
    MOCK_METHOD(double, libinput_event_pointer_get_scroll_value, (libinput_event_pointer* event, libinput_pointer_axis axis));
    MOCK_METHOD(int, libinput_event_pointer_has_axis, (libinput_event_pointer *,libinput_pointer_axis), ());

    MOCK_METHOD(uint32_t, libinput_event_touch_get_time, (libinput_event_touch*), ());
    MOCK_METHOD(uint64_t, libinput_event_touch_get_time_usec, (libinput_event_touch*), ());
    MOCK_METHOD(int32_t, libinput_event_touch_get_slot, (libinput_event_touch*), ());
    MOCK_METHOD(int32_t, libinput_event_touch_get_seat_slot, (libinput_event_touch*), ());
    MOCK_METHOD(double, libinput_event_touch_get_x, (libinput_event_touch*), ());
    MOCK_METHOD(double, libinput_event_touch_get_y, (libinput_event_touch*), ());
    MOCK_METHOD(double, libinput_event_touch_get_x_transformed, (libinput_event_touch*, uint32_t), ());
    MOCK_METHOD(double, libinput_event_touch_get_y_transformed, (libinput_event_touch*, uint32_t), ());
    MOCK_METHOD(double, libinput_event_touch_get_pressure, (libinput_event_touch*), ());
    MOCK_METHOD(double, libinput_event_touch_get_minor, (libinput_event_touch*), ());
    MOCK_METHOD(double, libinput_event_touch_get_minor_transformed, (libinput_event_touch*, uint32_t, uint32_t), ());
    MOCK_METHOD(double, libinput_event_touch_get_major, (libinput_event_touch*), ());
    MOCK_METHOD(double, libinput_event_touch_get_major_transformed, (libinput_event_touch*, uint32_t, uint32_t), ());
    MOCK_METHOD(double, libinput_event_touch_get_orientation, (libinput_event_touch*), ());

    MOCK_METHOD(libinput*, libinput_udev_create_context, (const libinput_interface *, void*, struct udev* udev), ());
    MOCK_METHOD(int, libinput_udev_assign_seat, (const libinput*, char const* seat), ());
    MOCK_METHOD(libinput*, libinput_path_create_context, (const libinput_interface *, void*), ());
    MOCK_METHOD(libinput_device*, libinput_path_add_device, (const libinput*, const char*), ());
    MOCK_METHOD(void, libinput_path_remove_device, (libinput_device*), ());

    MOCK_METHOD(libinput_device*, libinput_device_unref, (libinput_device*), ());
    MOCK_METHOD(libinput_device*, libinput_device_ref, (libinput_device*), ());
    MOCK_METHOD(char const*, libinput_device_get_name, (libinput_device*), ());
    MOCK_METHOD(udev_device*, libinput_device_get_udev_device, (libinput_device*), ());
    MOCK_METHOD(unsigned int, libinput_device_get_id_product, (libinput_device*), ());
    MOCK_METHOD(unsigned int, libinput_device_get_id_vendor, (libinput_device*), ());
    MOCK_METHOD(char const*, libinput_device_get_sysname, (libinput_device*), ());
    MOCK_METHOD(libinput_device_group*, libinput_device_get_device_group, (libinput_device*), ());

    MOCK_METHOD(int, libinput_device_config_tap_get_finger_count, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_tap_set_enabled, (libinput_device*,  libinput_config_tap_state enable), ());
    MOCK_METHOD(libinput_config_tap_state, libinput_device_config_tap_get_enabled, (libinput_device*), ());
    MOCK_METHOD(libinput_config_tap_state, libinput_device_config_tap_get_default_enabled, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_tap_set_drag_lock_enabled, (libinput_device*,  libinput_config_drag_lock_state enable), ());
    MOCK_METHOD(libinput_config_drag_lock_state, libinput_device_config_tap_get_drag_lock_enabled, (libinput_device*), ());
    MOCK_METHOD(libinput_config_drag_lock_state, libinput_device_config_tap_get_default_drag_lock_enabled, (libinput_device*), ());
    MOCK_METHOD(int, libinput_device_config_calibration_has_matrix, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_calibration_set_matrix, (libinput_device*, const float matrix[6]), ());
    MOCK_METHOD(int, libinput_device_config_calibration_get_matrix, (libinput_device*,  float matrix[6]), ());
    MOCK_METHOD(int, libinput_device_config_calibration_get_default_matrix, (libinput_device*,  float matrix[6]), ());
    MOCK_METHOD(uint32_t, libinput_device_config_send_events_get_modes, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_send_events_set_mode, (libinput_device*, uint32_t mode), ());
    MOCK_METHOD(uint32_t, libinput_device_config_send_events_get_mode, (libinput_device*), ());
    MOCK_METHOD(uint32_t, libinput_device_config_send_events_get_default_mode, (libinput_device*), ());
    MOCK_METHOD(int, libinput_device_config_accel_is_available, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_accel_set_speed, (libinput_device*, double speed), ());
    MOCK_METHOD(double, libinput_device_config_accel_get_speed, (libinput_device*), ());
    MOCK_METHOD(double, libinput_device_config_accel_get_default_speed, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_accel_set_profile, (libinput_device*, libinput_config_accel_profile), ());
    MOCK_METHOD(libinput_config_accel_profile, libinput_device_config_accel_get_profile, (libinput_device*), ());
    MOCK_METHOD(int, libinput_device_config_scroll_has_natural_scroll, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_scroll_set_natural_scroll_enabled, (libinput_device*, int enable), ());
    MOCK_METHOD(int, libinput_device_config_scroll_get_natural_scroll_enabled, (libinput_device*), ());
    MOCK_METHOD(int, libinput_device_config_scroll_get_default_natural_scroll_enabled, (libinput_device*), ());
    MOCK_METHOD(int, libinput_device_config_left_handed_is_available, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_left_handed_set, (libinput_device*,  int left_handed), ());
    MOCK_METHOD(int, libinput_device_config_left_handed_get, (libinput_device*), ());
    MOCK_METHOD(int, libinput_device_config_left_handed_get_default, (libinput_device*), ());
    MOCK_METHOD(uint32_t, libinput_device_config_click_get_methods, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_click_set_method, (libinput_device*, libinput_config_click_method method), ());
    MOCK_METHOD(libinput_config_click_method, libinput_device_config_click_get_method, (libinput_device*), ());
    MOCK_METHOD(libinput_config_click_method, libinput_device_config_click_get_default_method, (libinput_device*), ());
    MOCK_METHOD(int, libinput_device_config_middle_emulation_is_available, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_middle_emulation_set_enabled, (libinput_device*, libinput_config_middle_emulation_state enable), ());
    MOCK_METHOD(libinput_config_middle_emulation_state, libinput_device_config_middle_emulation_get_enabled, (libinput_device*), ());
    MOCK_METHOD(libinput_config_middle_emulation_state, libinput_device_config_middle_emulation_get_default_enabled, (libinput_device*), ());
    MOCK_METHOD(uint32_t, libinput_device_config_scroll_get_methods, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_scroll_set_method, (libinput_device*, libinput_config_scroll_method method), ());
    MOCK_METHOD(libinput_config_scroll_method, libinput_device_config_scroll_get_method, (libinput_device*), ());
    MOCK_METHOD(libinput_config_scroll_method, libinput_device_config_scroll_get_default_method, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_scroll_set_button, (libinput_device*,  uint32_t button), ());
    MOCK_METHOD(uint32_t, libinput_device_config_scroll_get_button, (libinput_device*), ());
    MOCK_METHOD(uint32_t, libinput_device_config_scroll_get_default_button, (libinput_device*), ());
    MOCK_METHOD(int, libinput_device_config_dwt_is_available, (libinput_device*), ());
    MOCK_METHOD(libinput_config_status, libinput_device_config_dwt_set_enabled, (libinput_device*, libinput_config_dwt_state enable), ());
    MOCK_METHOD(libinput_config_dwt_state, libinput_device_config_dwt_get_enabled, (libinput_device*), ());
    MOCK_METHOD(libinput_config_dwt_state, libinput_device_config_dwt_get_default_enabled, (libinput_device*), ());
    MOCK_METHOD(void, libinput_device_led_update, (libinput_device*, libinput_led), ());

    template<typename PtrT>
    PtrT get_next_fake_ptr()
    {
        return reinterpret_cast<PtrT>(++last_fake_ptr);
    }

    dev_t get_next_devnum()
    {
        auto const next_devnum =
            makedev(major(last_devnum) + 1, minor(last_devnum) + 1);
        last_devnum = next_devnum;
        return next_devnum;
    }

    std::vector<libinput_event*> events;
private:
    dispatch::ActionQueue libinput_simulation_queue;
    unsigned int last_fake_ptr{0};
    dev_t last_devnum{makedev(1,0)};
    void push_back(libinput_event* event);
};

}
}
}

#endif
