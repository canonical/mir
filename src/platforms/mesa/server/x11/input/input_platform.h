/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */
#ifndef MIR_INPUT_X_INPUT_PLATFORM_H_
#define MIR_INPUT_X_INPUT_PLATFORM_H_

#include "mir/input/platform.h"
#include <memory>
#include <X11/Xlib.h>

namespace mir
{

namespace dispatch
{
class ReadableFd;
}

namespace input
{

namespace X
{
class XInputDevice;

class XInputPlatform : public input::Platform
{
public:
    explicit XInputPlatform(
        std::shared_ptr<input::InputDeviceRegistry> const& input_device_registry,
        std::shared_ptr<::Display> const& conn);
    ~XInputPlatform() = default;

    std::shared_ptr<dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;

private:
    void process_input_event();
    std::shared_ptr<::Display> x11_connection;
    std::shared_ptr<dispatch::ReadableFd> const xcon_dispatchable;
    std::shared_ptr<input::InputDeviceRegistry> const registry;
    std::shared_ptr<XInputDevice> const core_keyboard;
    std::shared_ptr<XInputDevice> const core_pointer;
    bool kbd_grabbed;
    bool ptr_grabbed;
};

}
}
}

#endif // MIR_INPUT_X_INPUT_PLATFORM_H_
