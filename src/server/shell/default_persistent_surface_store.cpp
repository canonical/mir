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

msh::detail::UUID::UUID(std::string const& string_repr)
{
    uuid_parse(string_repr.c_str(), value);
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

std::string msh::detail::UUID::serialise_to_string() const
{
    char buf[37];
    uuid_unparse(value, buf);
    return buf;
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
    -> Id const&
{
    auto& surface_id = surface_to_id[surface.get()];
    if (surface_id)
    {
        return *surface_id;
    }
    else
    {
        auto new_element = id_to_surface.emplace(std::make_pair(detail::UUID{}, surface));
        surface_id = &new_element.first->first;
        return *surface_id;
    }
}

std::shared_ptr<ms::Surface> msh::DefaultPersistentSurfaceStore::surface_for_id(Id const& id)
{
    auto uuid = dynamic_cast<detail::UUID const*>(&id);
    if (uuid != nullptr)
    {
        return id_to_surface.at(*uuid);
    }
    return {};
}

auto msh::DefaultPersistentSurfaceStore::deserialise(std::string const& string_repr) const
    -> Id const&
{
    detail::UUID uuid{string_repr};

    auto tmp = id_to_surface.at(uuid);
    return *surface_to_id.at(tmp.get());
}
