/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_INPUT_KEYBOARD_RESYNC_DISPATCHER_H_
#define MIR_INPUT_KEYBOARD_RESYNC_DISPATCHER_H_

#include <mir/input/input_dispatcher.h>

#include <vector>

namespace mir
{
namespace input
{
class Seat;

class KeyboardResyncDispatcher : public InputDispatcher
{
public:
    KeyboardResyncDispatcher(std::shared_ptr<InputDispatcher> const& next_dispatcher);

    /// InputDispatcher overrides
    /// @{
    bool dispatch(std::shared_ptr<MirEvent const> const& event) override;
    void start() override;
    void stop() override;
    /// @}

private:
    std::shared_ptr<InputDispatcher> const next_dispatcher;
};

}
}

#endif // MIR_INPUT_KEYBOARD_RESYNC_DISPATCHER_H_
