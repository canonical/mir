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


#ifndef MIR_GRAPHICS_VIRTUAL_INPUT_PLATFORM_H_
#define MIR_GRAPHICS_VIRTUAL_INPUT_PLATFORM_H_

#include "mir/input/platform.h"

namespace mir
{
namespace input
{
namespace virt
{

/// Serves as a pass-through platform so that the virtual input platform
/// does not default to evdev (which would lead to funny things, like
/// two inputs being registered when using a VNC on localhost)
class VirtualInputPlatform : public input::Platform
{
public:
    VirtualInputPlatform();
    std::shared_ptr<dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;
};

}
}
}

#endif //MIR_GRAPHICS_VIRTUAL_INPUT_PLATFORM_H_
