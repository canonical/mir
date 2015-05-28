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

#include "mir/shell/persistent_surface_store.h"

#include <boost/throw_exception.hpp>

namespace msh = mir::shell;
using Id = mir::shell::PersistentSurfaceStore::Id;

Id::Id()
{
    uuid_generate(uuid);
}


Id::Id(Id const& rhs)
{
    std::copy(rhs.uuid, rhs.uuid + sizeof(rhs.uuid), uuid);
}

/*
Id& operator=(Id const& rhs);
*/

bool Id::operator==(Id const& rhs) const
{
    return uuid_compare(uuid, rhs.uuid) == 0;
}

std::vector<uint8_t> Id::serialize_id() const
{
    // uuid_unparse adds a trailing null; allocate enough memory for it...
    std::vector<uint8_t> buffer(37);
    uuid_unparse(uuid, reinterpret_cast<char*>(buffer.data()));
    // ...and then strip off the trailing null.
    buffer.resize(36);
    return buffer;
}


Id Id::deserialize_id(std::vector<uint8_t> const& buffer)
{
    if (buffer.size() != 36)
    {
        using namespace std::literals::string_literals;
        std::string buffer_as_string{buffer.begin(), buffer.end()};
        BOOST_THROW_EXCEPTION((std::invalid_argument{"Failed to parse: "s + buffer_as_string}));
    }
    std::array<char, 37> null_terminated_buffer;
    std::copy(buffer.begin(), buffer.end(), null_terminated_buffer.data());
    null_terminated_buffer[36] = '\0';
    return Id{null_terminated_buffer};
}

Id::Id(std::array<char, 37> const& buffer)
{
    if (uuid_parse(buffer.data(), uuid) != 0)
    {
        using namespace std::literals::string_literals;
        BOOST_THROW_EXCEPTION((std::invalid_argument{"Failed to parse: "s + buffer.data()}));
    }
}

auto std::hash<Id>::operator()(argument_type const &uuid) const
    -> result_type
{
    // uuid_t is defined in the header to be a char[16]; hash this as two 64-byte integers
    // and mix the result with XOR.
    return std::hash<uint64_t>()(*reinterpret_cast<uint64_t const*>(uuid.uuid)) ^
           std::hash<uint64_t>()(*reinterpret_cast<uint64_t const*>(uuid.uuid + sizeof(uint64_t)));
}
