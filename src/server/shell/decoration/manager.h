/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_DECORATION_MANAGER_H_
#define MIR_SHELL_DECORATION_MANAGER_H_

#include <memory>

namespace mir
{
namespace scene
{
class Surface;
class Session;
}
namespace shell
{
class Shell;
namespace decoration
{
/// Creates server-side decorations (SSDs) for windows upon request
class Manager
{
public:
    Manager() = default;
    virtual ~Manager() = default;

    /// Called by the server configuration when the shell is created
    /// shell can't be a construction parameter because the shell needs to be constructed with a manager
    virtual void init(std::weak_ptr<shell::Shell> const& shell) = 0;

    /// Decorates the window
    virtual void decorate(std::shared_ptr<scene::Surface> const& surface) = 0;

    /// Removes decorations from window if present
    /// If window is not currently decorated, does nothing
    virtual void undecorate(std::shared_ptr<scene::Surface> const& surface) = 0;

    /// Removes decorations from all currently decorated windows
    virtual void undecorate_all() = 0;

private:
    Manager(Manager const&) = delete;
    Manager& operator=(Manager const&) = delete;
};
}
}
}

#endif /* MIR_SHELL_DECORATION_MANAGER_H_ */
