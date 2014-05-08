/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_DISPATCHER_CONFIGURATION_H_
#define MIR_INPUT_INPUT_DISPATCHER_CONFIGURATION_H_

#include <memory>

namespace mir
{
namespace compositor
{
class Scene;
}
namespace shell
{
class InputTargeter;
}
namespace input
{
class InputTargets;
class InputDispatcher;

class InputDispatcherConfiguration
{
public:
    virtual ~InputDispatcherConfiguration() = default;

    virtual std::shared_ptr<shell::InputTargeter> the_input_targeter() = 0;
    virtual std::shared_ptr<input::InputDispatcher> the_input_dispatcher() = 0;

    virtual bool is_key_repeat_enabled() const = 0;

protected:
    InputDispatcherConfiguration() = default;
    InputDispatcherConfiguration(InputDispatcherConfiguration const&) = delete;
    InputDispatcherConfiguration& operator=(InputDispatcherConfiguration const&) = delete;
};
}
} // namespace mir

#endif // MIR_INPUT_INPUT_DISPATCHER_CONFIGURATION_H_
