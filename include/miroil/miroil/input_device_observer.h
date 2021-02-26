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
#ifndef MIROIL_INPUT_DEVICE_OBSERVER_H
#define MIROIL_INPUT_DEVICE_OBSERVER_H
#include <mir_toolkit/mir_input_device_types.h>
#include <memory>
#include <string>

namespace mir { namespace input { class Device; } }

namespace miroil
{
class InputDeviceObserver
{
public:
    virtual ~InputDeviceObserver();
    
    void             apply_keymap(const std::shared_ptr<mir::input::Device> &device, const std::string & layout, const std::string & variant);
    MirInputDeviceId get_device_id(const std::shared_ptr<mir::input::Device> &device);
    std::string      get_device_name(const std::shared_ptr<mir::input::Device> &device);
    bool             is_keyboard(const std::shared_ptr<mir::input::Device> &device);
    bool             is_alpha_numeric(const std::shared_ptr<mir::input::Device> &device);
    
    virtual void device_added(const std::shared_ptr<mir::input::Device> &device) = 0;
    virtual void device_removed(const std::shared_ptr<mir::input::Device> &device) = 0;
};
    
}

#endif //MIROIL_INPUT_DEVICE_OBSERVER_H
