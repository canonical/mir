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

#ifndef MIR_SHELL_FOCUS_CONTROLLER_H_
#define MIR_SHELL_FOCUS_CONTROLLER_H_

#include <memory>

namespace mir
{
namespace scene { class Session; }

namespace shell
{
// TODO I don't think this interface serves a meaningful purpose
// TODO (It is referenced by a couple of example WindowManagers, and
// TODO to get the active session in unity-system-compositor.)
// TODO I think there's a better approach possible.
class FocusController
{
public:
    virtual ~FocusController() = default;

    virtual std::weak_ptr<scene::Session> focussed_application() const = 0;
    virtual void focus_next() = 0;
    virtual void set_focus_to(std::shared_ptr<scene::Session> const& focus) = 0;

protected:
    FocusController() = default;
    FocusController(FocusController const&) = delete;
    FocusController& operator=(FocusController const&) = delete;
};

}
} // namespace mir

#endif // MIR_SHELL_FOCUS_CONTROLLER_H_
