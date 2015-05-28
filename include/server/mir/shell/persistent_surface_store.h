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
#include <vector>
#include <array>
#include <uuid/uuid.h>


namespace mir
{
namespace scene
{
class Surface;
}

namespace shell
{
/**
 * A store for Surface information divorced from the lifetime of any given Session
 *
 * This provides the backing for persistent references to Surface objects,
 * both across client Sessions and across shell restarts, for features such as
 * surface position/size restoration.
 *
 * \todo Persistence across shell restarts is as-yet unimplemented.
 */
class PersistentSurfaceStore
{
public:
    class Id;

    virtual ~PersistentSurfaceStore() = default;

    /**
     * \brief Acquire ID for a Surface
     * \param [in]    surface to query or generate an ID for
     * \return        A reference to the ID of this surface. This reference is stable
     *                for the lifetime of the PersistentSurfaceStore.
     * \note If \arg surface has not yet had an ID generated, this generates its ID.
     */
    virtual Id id_for_surface(std::shared_ptr<scene::Surface> const& surface) = 0;
    /**
     * \brief Lookup Surface by ID.
     * \param [in] id    ID of surface to lookup
     * \return           The surface with ID \arg id.
     */
    virtual std::shared_ptr<scene::Surface> surface_for_id(Id const& id) const = 0;
};
}
}

namespace std
{
template<>
struct hash<mir::shell::PersistentSurfaceStore::Id>;
}

namespace mir
{
namespace shell
{

class PersistentSurfaceStore::Id final
{
public:
    Id();

    Id(Id const& rhs);
    Id& operator=(Id const& rhs);

    bool operator==(Id const& rhs) const;

    std::vector<uint8_t> serialize_id() const;
    static Id deserialize_id(std::vector<uint8_t> const& buffer);

private:
    friend struct std::hash<Id>;

    Id(std::array<char, 37> const& buffer);

    uuid_t uuid;
};

}
}

namespace std
{
template<>
struct hash<mir::shell::PersistentSurfaceStore::Id>
{
    typedef mir::shell::PersistentSurfaceStore::Id argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const& uuid) const;
};
}

#endif // MIR_SHELL_PERSISTENT_SURFACE_STORE_H_
