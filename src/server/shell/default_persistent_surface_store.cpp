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

#include "default_persistent_surface_store.h"
#include <uuid/uuid.h>
#include <algorithm>
#include <unordered_map>
#include <boost/throw_exception.hpp>

namespace msh = mir::shell;
namespace ms = mir::scene;

class msh::DefaultPersistentSurfaceStore::SurfaceIdBimap
{
public:
    using Id = PersistentSurfaceStore::Id;

    Id const& insert_or_retrieve(std::shared_ptr<scene::Surface> const& surface);

    std::shared_ptr<scene::Surface> operator[](Id const& id) const;
    Id const& operator[](std::shared_ptr<scene::Surface> const& surface) const;

private:
    std::unordered_map<Id, std::weak_ptr<scene::Surface>> id_to_surface;
    std::unordered_map<scene::Surface const*, Id const*> surface_to_id;
};

auto msh::DefaultPersistentSurfaceStore::SurfaceIdBimap::insert_or_retrieve(std::shared_ptr<scene::Surface> const& surface)
    -> Id const&
{
    auto element = surface_to_id.insert(std::make_pair(surface.get(), nullptr));
    if (!element.second)
    {
        // Surface already exists in our map, return it.
        return *element.first->second;
    }
    else
    {
        auto new_element = id_to_surface.insert(std::make_pair(Id{}, surface));
        // Update the surface_to_id map element from nullptr to our new Id
        element.first->second = &new_element.first->first;
        return *element.first->second;
    }
}

auto msh::DefaultPersistentSurfaceStore::SurfaceIdBimap::operator[](std::shared_ptr<scene::Surface> const& surface) const
    -> Id const&
{
    return *surface_to_id.at(surface.get());
}

auto msh::DefaultPersistentSurfaceStore::SurfaceIdBimap::operator[](Id const& id) const
    -> std::shared_ptr<scene::Surface>
{
    return id_to_surface.at(id).lock();
}

msh::DefaultPersistentSurfaceStore::DefaultPersistentSurfaceStore()
    : store{std::make_unique<SurfaceIdBimap>()}
{
}

msh::DefaultPersistentSurfaceStore::~DefaultPersistentSurfaceStore()
{
}

auto msh::DefaultPersistentSurfaceStore::id_for_surface(std::shared_ptr<scene::Surface> const& surface)
    -> Id
{
    std::lock_guard<std::mutex> lock{mutex};
    return store->insert_or_retrieve(surface);
}

std::shared_ptr<ms::Surface> msh::DefaultPersistentSurfaceStore::surface_for_id(Id const& id) const
{
    try
    {
        std::lock_guard<std::mutex> lock{mutex};
        return (*store)[id];
    }
    catch (std::out_of_range& err)
    {
        using namespace std::literals;
        BOOST_THROW_EXCEPTION(std::out_of_range(
            "Lookup for surface with ID: "s + id.serialize_to_string() + " failed."));
    }
}
