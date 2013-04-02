/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Daniel d'Andradra <daniel.dandrada@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_MANAGER_H_
#define MIR_INPUT_INPUT_MANAGER_H_

#include "mir/input/input_channel_factory.h"
#include "mir/shell/input_focus_selector.h"

#include <memory>

namespace mir
{
namespace graphics
{
class ViewableArea;
}

namespace input
{
class EventFilter;
class InputChannel;
class SessionTarget;
class SurfaceTarget;

class InputManager : public InputChannelFactory, public shell::InputFocusSelector
{
public:
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual std::shared_ptr<InputChannel> make_input_channel() = 0;

    virtual void set_input_focus_to(std::shared_ptr<input::SessionTarget> const& session,
                                    std::shared_ptr<input::SurfaceTarget> const& surface) = 0;

protected:
    InputManager() {};
    virtual ~InputManager() {}
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
};

// Needs to be implemented by platform/stack-specific code
std::shared_ptr<InputManager> create_input_manager(
    const std::initializer_list<std::shared_ptr<input::EventFilter> const>& event_filters,
    std::shared_ptr<graphics::ViewableArea> const& viewable_area);

}
}

#endif // MIR_INPUT_INPUT_MANAGER
