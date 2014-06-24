/*
 * Copyright © 2012 Canonical Ltd.
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
 *              Daniel d'Andradra <daniel.dandrada@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_TARGETS_H_
#define MIR_INPUT_INPUT_TARGETS_H_

#include "mir/input/input_channel_factory.h"

#include <memory>

namespace mir
{
namespace scene
{
class Observer;
}

namespace input
{
class Surface;

class InputTargets
{
public:
    virtual ~InputTargets() = default;

    virtual void for_each(std::function<void(std::shared_ptr<input::Surface> const&)> const& callback) = 0;

    virtual void add_observer(std::shared_ptr<scene::Observer> const& observer) = 0;
    virtual void remove_observer(std::weak_ptr<scene::Observer> const& observer) = 0;

protected:
    InputTargets() = default;
    InputTargets(InputTargets const&) = delete;
    InputTargets& operator=(InputTargets const&) = delete;
};

}
}

#endif // MIR_INPUT_INPUT_TARGETS
