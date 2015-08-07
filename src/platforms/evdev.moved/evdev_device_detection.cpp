/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include "evdev_device_detection.h"
#include "mir/fd.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/throw_exception.hpp>

#include <linux/input.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstring>
#include <system_error>

namespace mi = mir::input;
namespace mie = mi::evdev;

namespace
{

struct DeviceInfo
{
    uint8_t key_bit_mask[(KEY_MAX+1)/8];
    uint8_t abs_bit_mask[(ABS_MAX+1)/8];
    uint8_t rel_bit_mask[(REL_MAX+1)/8];
    uint8_t sw_bit_mask[(SW_MAX+1)/8];
    uint8_t property_bit_mask[(INPUT_PROP_MAX+1)/8];
};

void fill_device_info(DeviceInfo& info, int fd)
{
    std::memset(&info, 0, sizeof info);

    auto const get_bitmask = [&](int bit, size_t size, uint8_t* buf) -> void
    {
        if(ioctl(fd, EVIOCGBIT(bit, size), buf) < 1)
            BOOST_THROW_EXCEPTION(
                std::system_error(std::error_code(errno, std::system_category()),
                                  "Failed to query input device"));
    };
    get_bitmask(EV_KEY, sizeof info.key_bit_mask, info.key_bit_mask);
    get_bitmask(EV_REL, sizeof info.rel_bit_mask, info.rel_bit_mask);
    get_bitmask(EV_ABS, sizeof info.abs_bit_mask, info.abs_bit_mask);
    get_bitmask(EV_SW, sizeof info.sw_bit_mask, info.sw_bit_mask);

    if (ioctl(fd, EVIOCGPROP(sizeof info.property_bit_mask), info.property_bit_mask) < 1)
            BOOST_THROW_EXCEPTION(
                std::system_error(std::error_code(errno, std::system_category()),
                                  "Failed to query devices properties"));

}

constexpr size_t index_of_bit(size_t bit)
{
    return (bit + 7) / 8;
}
inline bool get_bit(uint8_t const* array, size_t bit)
{
    return array[bit/8] & (1<<(bit%8));
}

inline size_t get_num_bits(uint8_t const* array, std::initializer_list<size_t> bits)
{
    size_t ret = 0;
    for( auto const bit : bits)
        ret += get_bit(array, bit);
    return ret;
}

mi::DeviceCapabilities evaluate_device_capabilities(DeviceInfo const& info)
{
    mi::DeviceCapabilities caps;

    auto const contains_non_zero = [](uint8_t const* array, int first, int last) -> bool
    {
        return std::any_of(array + first, array + last, [](uint8_t item) { return item!=0;});
    };

    bool const has_keys = contains_non_zero(info.key_bit_mask, 0, index_of_bit(BTN_MISC))
        || contains_non_zero(info.key_bit_mask, index_of_bit(KEY_OK), sizeof info.key_bit_mask);

    bool const has_gamepad_buttons =
        contains_non_zero(info.key_bit_mask, index_of_bit(BTN_MISC), index_of_bit(BTN_MOUSE))
        || contains_non_zero(info.key_bit_mask, index_of_bit(BTN_JOYSTICK), index_of_bit(BTN_DIGI));

    bool const has_coordinates = get_bit(info.abs_bit_mask, ABS_X) &&
        get_bit(info.abs_bit_mask, ABS_Y);

    bool const has_mt_coordinates = get_bit(info.abs_bit_mask, ABS_MT_POSITION_X) &&
        get_bit(info.abs_bit_mask, ABS_MT_POSITION_Y);

    bool const is_direct = get_bit(info.property_bit_mask, INPUT_PROP_DIRECT);

    bool const finger_but_no_pen = get_bit(info.key_bit_mask, BTN_TOOL_FINGER)
        && !get_bit(info.key_bit_mask, BTN_TOOL_PEN);

    bool const has_touch = get_bit(info.key_bit_mask, BTN_TOUCH);

    bool const has_joystick_axis = 0 < get_num_bits(
        info.abs_bit_mask, {ABS_Z,
        ABS_RX, ABS_RY, ABS_RZ,
        ABS_THROTTLE, ABS_RUDDER, ABS_WHEEL, ABS_GAS, ABS_BRAKE,
        ABS_HAT0X, ABS_HAT0Y, ABS_HAT1X, ABS_HAT1Y, ABS_HAT2X, ABS_HAT2Y, ABS_HAT3X, ABS_HAT3Y,
        ABS_TILT_X, ABS_TILT_Y
        });

    if (has_keys || has_gamepad_buttons)
        caps = caps | mi::DeviceCapability::keyboard;

    if (get_bit(info.key_bit_mask, BTN_MOUSE) && get_bit(info.rel_bit_mask, REL_X) && get_bit(info.rel_bit_mask, REL_Y))
        caps = caps | mi::DeviceCapability::pointer;

    if (finger_but_no_pen && !is_direct && (has_coordinates|| has_mt_coordinates))
        caps = caps | mi::DeviceCapability::touchpad;
    else if (has_touch && ((has_mt_coordinates && !has_gamepad_buttons)
                           || has_coordinates))
        caps = caps | mi::DeviceCapability::touchscreen;

    if (has_joystick_axis || (!has_touch && has_coordinates))
        caps = caps | mi::DeviceCapability::joystick;

    if (has_gamepad_buttons)
        caps = caps | mi::DeviceCapability::gamepad;

    return caps;
}

}

mi::DeviceCapabilities mie::detect_device_capabilities(char const* device)
{
    mir::Fd input_device(::open(device, O_RDONLY|O_NONBLOCK));
    if (input_device < 0)
        BOOST_THROW_EXCEPTION(
            std::system_error(std::error_code(errno, std::system_category()),
                              "Failed to open input device"));

    DeviceInfo info;

    fill_device_info(info, input_device);
    return evaluate_device_capabilities(info);
}

