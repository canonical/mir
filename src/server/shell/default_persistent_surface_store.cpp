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
#include <algorithm>

namespace msh = mir::shell;
namespace ms = mir::scene;

class UUID : public msh::PersistentSurfaceStore::Id
{
public:
    friend class msh::PersistentSurfaceStore;

    UUID();
    UUID(std::string const& id);

    bool operator==(Id const& rhs) const override;

    operator std::string() const;
private:
    uuid_t value;
};

UUID::UUID()
{
    uuid_generate(value);
}

UUID::UUID(std::string const& string_repr)
{
    if (uuid_parse(string_repr.c_str(), value) != 0)
    {
        throw std::runtime_error{string_repr.data()};
    }
}

bool UUID::operator==(msh::PersistentSurfaceStore::Id const& rhs) const
{
    auto rhs_resolved = dynamic_cast<UUID const*>(&rhs);
    if (rhs_resolved)
    {
        return !uuid_compare(value, rhs_resolved->value);
    }
    return false;
}

UUID::operator std::string() const
{
    char buffer[37];
    uuid_unparse_upper(value, buffer);
    return buffer;
}

msh::DefaultPersistentSurfaceStore::DefaultPersistentSurfaceStore()
{
}

auto msh::DefaultPersistentSurfaceStore::id_for_surface(std::shared_ptr<scene::Surface> const& surface)
    -> std::unique_ptr<Id>
{
    auto prexistent = std::find_if(store.cbegin(), store.cend(),
                                   [&surface](auto candidate)
    {
        return candidate.second == surface;
    });
    if (prexistent != store.cend())
    {
        return std::make_unique<UUID>(prexistent->first);
    }
    else
    {
        auto new_id = std::make_unique<UUID>();
        store[*new_id] = surface;
        return std::move(new_id);
    }
}

std::shared_ptr<ms::Surface> msh::DefaultPersistentSurfaceStore::surface_for_id(Id const& id)
{
    auto uuid = dynamic_cast<UUID const*>(&id);
    if (uuid != nullptr)
    {
        return store.at(*uuid);
    }
    return {};
}
