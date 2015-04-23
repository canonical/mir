/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_SHELL_PERSISTENT_SURFACE_STORE_H_
#define MIR_SHELL_PERSISTENT_SURFACE_STORE_H_

#include <memory>

namespace mir
{
namespace scene
{
class Surface;
}

namespace shell
{
class PersistentSurfaceStore
{
public:
    class Id
    {
    public:
        virtual ~Id() = default;
        virtual bool operator==(Id const& rhs) const = 0;
    };

    virtual ~PersistentSurfaceStore() = default;

    virtual std::unique_ptr<Id> id_for_surface(std::shared_ptr<scene::Surface> const& surface) = 0;
    virtual std::shared_ptr<scene::Surface> surface_for_id(Id const& id) = 0;

};
}
}

#endif // MIR_SHELL_PERSISTENT_SURFACE_STORE_H_
