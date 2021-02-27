/*
 * Copyright (C) 2016-2021 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MIROIL_INPUT_DEVICE_H
#define MIROIL_INPUT_DEVICE_H
#include <mir_toolkit/mir_input_device_types.h>
#include <memory>
#include <string>

namespace mir { namespace input { class Device; } }

namespace miroil
{
class InputDevice
{
public:
    InputDevice(std::shared_ptr<mir::input::Device> const & device);    
    ~InputDevice();
    
    void             apply_keymap(std::string const & layout, std::string const & variant);
    MirInputDeviceId get_device_id();
    std::string      get_device_name();
    bool             is_keyboard();
    bool             is_alpha_numeric();

private:
    std::shared_ptr<mir::input::Device> const & device;
};
    
}

#endif //MIROIL_INPUT_DEVICE_H
