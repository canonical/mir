/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_SHELL_DEFAULT_PERSISTENT_SURFACE_STORE_H_
#define MIR_SHELL_DEFAULT_PERSISTENT_SURFACE_STORE_H_

#include "mir/shell/persistent_surface_store.h"
#include <mutex>

namespace mir
{
namespace shell
{
class DefaultPersistentSurfaceStore : public PersistentSurfaceStore
{
public:
    DefaultPersistentSurfaceStore();
    ~DefaultPersistentSurfaceStore() override;

    Id id_for_surface(std::shared_ptr<scene::Surface> const& surface) override;
    std::shared_ptr<scene::Surface> surface_for_id(Id const& id) const override;

private:
    class SurfaceIdBimap;
    std::mutex mutable mutex;
    std::unique_ptr<SurfaceIdBimap> const store;
};
}
}

#endif // MIR_SHELL_DEFAULT_PERSISTENT_SURFACE_STORE_H_
