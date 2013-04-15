/*
 * Copyright © 2013 Canonical Ltd.
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
 */

#ifndef MIR_SHELL_INPUT_FOCUS_SELECTOR_H_
#define MIR_SHELL_INPUT_FOCUS_SELECTOR_H_

#include <memory>

namespace mir
{
namespace input
{
class SessionTarget;
class SurfaceTarget;
}

namespace shell
{

class InputFocusSelector
{
public:
    virtual ~InputFocusSelector() {}

    virtual void set_input_focus_to(std::shared_ptr<input::SessionTarget> const& focus_application,
                                    std::shared_ptr<input::SurfaceTarget> const& focus_surface) = 0;

protected:
    InputFocusSelector() = default;
    InputFocusSelector(InputFocusSelector const&) = delete;
    InputFocusSelector& operator=(InputFocusSelector const&) = delete;
};

}
} // namespace mir

#endif // MIR_SHELL_INPUT_FOCUS_SELECTOR_H_
