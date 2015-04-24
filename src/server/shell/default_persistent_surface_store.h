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

#ifndef MIR_SHELL_DEFAULT_PERSISTENT_SURFACE_STORE_H_
#define MIR_SHELL_DEFAULT_PERSISTENT_SURFACE_STORE_H_

#include "mir/shell/persistent_surface_store.h"

#include <unordered_map>
#include <uuid/uuid.h>

namespace mir
{
namespace shell
{
namespace detail
{
class UUID;
}
}
}

namespace std
{
template<>
struct hash<mir::shell::detail::UUID>;
}

namespace mir
{
namespace shell
{
namespace detail
{
class UUID : public PersistentSurfaceStore::Id
{
public:
    UUID();
    UUID(std::string const& string_repr);
    UUID(UUID const& copy_from);

    bool operator==(Id const& rhs) const override;

    std::string serialise_to_string() const override;
private:
    uuid_t value;

    friend struct std::hash<UUID>;
};
}
}
}

namespace std
{
template<>
struct hash<mir::shell::detail::UUID>
{
    typedef mir::shell::detail::UUID argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const& uuid) const;
};
}


namespace mir
{
namespace shell
{
class DefaultPersistentSurfaceStore : public PersistentSurfaceStore
{
public:
    DefaultPersistentSurfaceStore();

    virtual Id const& id_for_surface(std::shared_ptr<scene::Surface> const& surface) override;
    virtual std::shared_ptr<scene::Surface> surface_for_id(Id const& id) override;

    virtual Id const& deserialise(std::string const& string_repr) const override;
private:
    std::unordered_map<detail::UUID, std::shared_ptr<scene::Surface>> id_to_surface;
    std::unordered_map<scene::Surface const*, detail::UUID const*> surface_to_id;
};
}
}

#endif // MIR_SHELL_DEFAULT_PERSISTENT_SURFACE_STORE_H_
