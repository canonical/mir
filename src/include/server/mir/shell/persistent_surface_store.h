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

#ifndef MIR_SHELL_PERSISTENT_SURFACE_STORE_H_
#define MIR_SHELL_PERSISTENT_SURFACE_STORE_H_

#include <memory>
#include <vector>
#include <array>
#include <uuid.h>


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
     * \param [in] surface Surface to query or generate an ID for
     * \return             The ID for this surface.
     * \note If \arg surface has not yet had an ID generated, this generates its ID.
     * \note This does not extend the lifetime of \arg surface.
     */
    virtual Id id_for_surface(std::shared_ptr<scene::Surface> const& surface) = 0;

    /**
     * \brief Lookup Surface by ID.
     * \param [in] id    ID of surface to lookup
     * \return           The surface with ID \arg id. If this surface has been destroyed,
     *                   but the store retains a reference, returns nullptr.
     * \throws std::out_of_range if the store has no reference for a surface with \arg id.
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
    /**
     * \brief Generate a new, unique Id.
     */
    Id();

    /**
     * \brief Construct an Id from its serialized string form
     * \param serialized_form [in] The previously-serialized Id
     * \throw std::invalid_argument if \arg serialized_form is not parseable as an Id.
     */
    Id(std::string const& serialized_form);

    Id(Id const& rhs);
    Id& operator=(Id const& rhs);

    bool operator==(Id const& rhs) const;

    /**
     * \brief Serialize to a UTF-8 string
     * \return A string representation of the Id; this is guaranteed to be valid UTF-8
     */
    std::string serialize_to_string() const;
private:
    friend struct std::hash<Id>;

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
