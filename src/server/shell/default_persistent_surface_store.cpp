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

msh::detail::UUID::UUID()
{
    uuid_generate(value);
}

msh::detail::UUID::UUID(UUID const& copy_from)
{
    std::copy(copy_from.value, copy_from.value + sizeof(copy_from.value), value);
}

bool msh::detail::UUID::operator==(msh::PersistentSurfaceStore::Id const& rhs) const
{
    auto rhs_resolved = dynamic_cast<UUID const*>(&rhs);
    if (rhs_resolved)
    {
        return !uuid_compare(value, rhs_resolved->value);
    }
    return false;
}

auto std::hash<msh::detail::UUID>::operator()(argument_type const& arg) const
    -> result_type
{
    return std::hash<uint64_t>()(*reinterpret_cast<uint64_t const*>(arg.value)) ^
           std::hash<uint64_t>()(*reinterpret_cast<uint64_t const*>(arg.value + 8));
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
        return std::make_unique<detail::UUID>(prexistent->first);
    }
    else
    {
        auto new_id = std::make_unique<detail::UUID>();
        store[*new_id] = surface;
        return std::move(new_id);
    }
}

std::shared_ptr<ms::Surface> msh::DefaultPersistentSurfaceStore::surface_for_id(Id const& id)
{
    auto uuid = dynamic_cast<detail::UUID const*>(&id);
    if (uuid != nullptr)
    {
        return store.at(*uuid);
    }
    return {};
}
