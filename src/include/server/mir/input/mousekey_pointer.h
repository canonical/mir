/*
 * Copyright © Canonical Ltd.
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
 */


#ifndef MIR_INPUT_MOUSEKEY_POINTER
#define MIR_INPUT_MOUSEKEY_POINTER

#include "mir/input/event_filter.h"
#include "mir/input/input_event_transformer.h"
#include "mir/input/virtual_input_device.h"
#include "mir/main_loop.h"

namespace mir
{
namespace input
{
class MousekeyPointer : public VirtualInputDevice, public EventFilter
{
public:
    MousekeyPointer(std::shared_ptr<MainLoop> main_loop, std::shared_ptr<InputEventTransformer> iet);
    bool handle(MirEvent const& event) override;

private:
    std::shared_ptr<MainLoop> const main_loop;
    std::shared_ptr<input::InputEventTransformer> const iet;
};

}
}

#endif // MIR_INPUT_MOUSEKEY_POINTER
