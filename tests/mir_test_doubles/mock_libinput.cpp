/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

namespace
{
mtd::MockLibInput* global_libinput = nullptr;
}

mtd::MockLibInput::MockLibInput()
{
    using namespace testing;
    assert(global_libinput == NULL && "Only one mock object per process is allowed");
    global_libinput = this;

    ON_CALL(*this, libinput_device_ref(_)).WillByDefault(ReturnArg<0>());
    ON_CALL(*this, libinput_get_fd(_)).WillByDefault(Return(int(libinput_simulation_queue.watch_fd())));
}

void mtd::MockLibInput::wake()
{
    libinput_simulation_queue.enqueue([]{});
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

double libinput_event_pointer_get_axis_value(libinput_event_pointer* event, libinput_pointer_axis axis)
{
    return global_libinput->libinput_event_pointer_get_axis_value(event, axis);
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

double libinput_event_touch_get_major(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_major(event);
}

double libinput_event_touch_get_minor(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_minor(event);
}

double libinput_event_touch_get_major_transformed(libinput_event_touch* event, uint32_t width, uint32_t height)
{
    return global_libinput->libinput_event_touch_get_major_transformed(event, width, height);
}

double libinput_event_touch_get_minor_transformed(libinput_event_touch* event, uint32_t width, uint32_t height)
{
    return global_libinput->libinput_event_touch_get_minor_transformed(event, width, height);
}

double libinput_event_touch_get_pressure(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_pressure(event);
}

double libinput_event_touch_get_orientation(libinput_event_touch* event)
{
    return global_libinput->libinput_event_touch_get_orientation(event);
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
