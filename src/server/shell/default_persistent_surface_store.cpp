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
#include <algorithm>
#include <unordered_map>

namespace msh = mir::shell;
namespace ms = mir::scene;

namespace
{
class UUID : public msh::PersistentSurfaceStore::Id
{
public:
    UUID();
    UUID(std::string const& string_repr);
    UUID(UUID const& copy_from);

    bool operator==(Id const& rhs) const override;

    std::size_t hash() const;
    std::vector<uint8_t> serialise() const;
private:
    uuid_t value;
};

UUID::UUID()
{
    uuid_generate(value);
}

UUID::UUID(std::string const& string_repr)
{
    uuid_parse(string_repr.c_str(), value);
}

UUID::UUID(UUID const& copy_from)
{
    std::copy(copy_from.value, copy_from.value + sizeof(copy_from.value), value);
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

std::size_t UUID::hash() const
{
    return std::hash<uint64_t>()(*reinterpret_cast<uint64_t const*>(value)) ^
           std::hash<uint64_t>()(*reinterpret_cast<uint64_t const*>(value + 8));
}

std::vector<uint8_t> UUID::serialise() const
{
    std::vector<uint8_t> buf(37);
    uuid_unparse(value, reinterpret_cast<char*>(buf.data()));
    return buf;
}
}

namespace std
{
template<>
struct hash<UUID>
{
    typedef UUID argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const& uuid) const
    {
        return uuid.hash();
    }
};
}

class msh::DefaultPersistentSurfaceStore::SurfaceIdBimap
{
public:
    UUID const& insert_or_retrieve(std::shared_ptr<scene::Surface> const& surface);

    std::shared_ptr<scene::Surface> operator[](UUID const& id) const;
    UUID const& operator[](std::shared_ptr<scene::Surface> const& surface) const;

private:
    std::unordered_map<UUID, std::shared_ptr<scene::Surface>> id_to_surface;
    std::unordered_map<scene::Surface const*, UUID const*> surface_to_id;
};

UUID const& msh::DefaultPersistentSurfaceStore::SurfaceIdBimap::insert_or_retrieve(std::shared_ptr<scene::Surface> const& surface)
{
    auto element = surface_to_id.insert(std::make_pair(surface.get(), nullptr));
    if (!element.second)
    {
        // Surface already exists in our map, return it.
        return *element.first->second;
    }
    else
    {
        auto new_element = id_to_surface.insert(std::make_pair(UUID{}, surface));
        // Update the surface_to_id map element from nullptr to our new UUID
        element.first->second = &new_element.first->first;
        return *element.first->second;
    }
}

UUID const& msh::DefaultPersistentSurfaceStore::SurfaceIdBimap::operator[](std::shared_ptr<scene::Surface> const& surface) const
{
    return *surface_to_id.at(surface.get());
}

auto msh::DefaultPersistentSurfaceStore::SurfaceIdBimap::operator[](UUID const& id) const
    -> std::shared_ptr<scene::Surface>
{
    return id_to_surface.at(id);
}

msh::DefaultPersistentSurfaceStore::DefaultPersistentSurfaceStore()
    : store{std::make_unique<SurfaceIdBimap>()}
{
}

msh::DefaultPersistentSurfaceStore::~DefaultPersistentSurfaceStore()
{
}

auto msh::DefaultPersistentSurfaceStore::id_for_surface(std::shared_ptr<scene::Surface> const& surface)
    -> Id const&
{
    return store->insert_or_retrieve(surface);
}

std::shared_ptr<ms::Surface> msh::DefaultPersistentSurfaceStore::surface_for_id(Id const& id)
{
    auto uuid = dynamic_cast<UUID const*>(&id);
    if (uuid != nullptr)
    {
        return (*store)[*uuid];
    }
    return {};
}

std::vector<uint8_t> msh::DefaultPersistentSurfaceStore::serialise_id(Id const& id) const
{
    return reinterpret_cast<UUID const&>(id).serialise();
}

auto msh::DefaultPersistentSurfaceStore::deserialise_id(std::vector<uint8_t> const& buf) const
    -> Id const&
{
    UUID uuid{reinterpret_cast<char const*>(buf.data())};

    auto surface = (*store)[uuid];
    return (*store)[surface];
}
