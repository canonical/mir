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

#include "mir/shell/persistent_surface_store.h"

#include <boost/throw_exception.hpp>

namespace msh = mir::shell;
using Id = mir::shell::PersistentSurfaceStore::Id;

Id::Id()
{
    uuid_generate(uuid);
}

Id::Id(std::string const& serialized_form)
{
    using namespace std::literals::string_literals;
    if (serialized_form.size() != 36)
    {
        BOOST_THROW_EXCEPTION((std::invalid_argument{"Failed to parse: "s + serialized_form +
            " (has invalid length: "s + std::to_string(serialized_form.size()) +
            " expected 36)"}));
    }

    if (uuid_parse(serialized_form.c_str(), uuid) != 0)
    {
        BOOST_THROW_EXCEPTION((std::invalid_argument{"Failed to parse: "s + serialized_form}));
    }
}

Id::Id(Id const& rhs)
{
    std::copy(rhs.uuid, rhs.uuid + sizeof(rhs.uuid), uuid);
}

Id& Id::operator=(Id const& rhs)
{
    std::copy(rhs.uuid, rhs.uuid + sizeof(rhs.uuid), uuid);
    return *this;
}

bool Id::operator==(Id const& rhs) const
{
    return uuid_compare(uuid, rhs.uuid) == 0;
}

std::string Id::serialize_to_string() const
{
    // uuid_unparse adds a trailing null; allocate enough memory for it...
    char buffer[37];
    uuid_unparse(uuid, buffer);

    return buffer;
}

auto std::hash<Id>::operator()(argument_type const &uuid) const
    -> result_type
{
    // uuid_t is defined in the header to be a char[16]; hash this as two 64-byte integers
    // and mix the result with XOR.
    return std::hash<uint64_t>()(*reinterpret_cast<uint64_t const*>(uuid.uuid)) ^
           std::hash<uint64_t>()(*reinterpret_cast<uint64_t const*>(uuid.uuid + sizeof(uint64_t)));
}
