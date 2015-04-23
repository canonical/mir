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

#include "default_persistent_surface_store.h"
#include <uuid/uuid.h>
#include <array>
#include <string.h>

namespace msh = mir::shell;
namespace ms = mir::scene;

class UUID : public msh::PersistentSurfaceStore::Id
{
public:
    UUID(std::string const& id);

    bool operator==(Id const& rhs) const override;

private:
    uuid_t id;
};

UUID::UUID(std::string const& string_repr)
{
    if (uuid_parse(string_repr.c_str(), id) != 0)
    {
        throw std::runtime_error{string_repr.data()};
    }
}

bool UUID::operator==(msh::PersistentSurfaceStore::Id const& rhs) const
{
    auto rhs_resolved = dynamic_cast<UUID const*>(&rhs);
    if (rhs_resolved)
    {
        return !uuid_compare(id, rhs_resolved->id);
    }
    return false;
}

msh::DefaultPersistentSurfaceStore::DefaultPersistentSurfaceStore()
{
}

auto msh::DefaultPersistentSurfaceStore::id_for_surface(std::shared_ptr<scene::Surface> const& /*surface*/)
    -> std::unique_ptr<Id>
{
    return std::make_unique<UUID>("499847d1-f351-46fc-9439-0918253a642c");
}

std::shared_ptr<ms::Surface> msh::DefaultPersistentSurfaceStore::surface_for_id(Id const& /*id*/)
{
    return {};
}
