/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SHELL_INPUT_TARGETER_H_
#define MIR_SHELL_INPUT_TARGETER_H_

#include <stddef.h>
#include <memory>
#include <vector>

namespace mir
{
namespace input
{
class Surface;
}

namespace shell
{

/// An interface used to control the selection of keyboard input focus.
class InputTargeter
{
public:
    virtual ~InputTargeter() = default;

    virtual void set_focus(std::shared_ptr<input::Surface> const& focus_surface) = 0;
    virtual void clear_focus() = 0;

    virtual void set_drag_and_drop_handle(std::vector<uint8_t> const& handle) = 0;
    virtual void clear_drag_and_drop_handle() = 0;

protected:
    InputTargeter() = default;
    InputTargeter(InputTargeter const&) = delete;
    InputTargeter& operator=(InputTargeter const&) = delete;
};

}
} // namespace mir

#endif // MIR_SHELL_INPUT_TARGETER_H_
