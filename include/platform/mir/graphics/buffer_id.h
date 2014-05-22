/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_GRAPHICS_BUFFER_ID_H_
#define MIR_GRAPHICS_BUFFER_ID_H_

#include <cstdint>

namespace mir
{
namespace graphics
{
class BufferID
{
public:
    BufferID() : value(id_invalid){}
    explicit BufferID(uint32_t val) : value(val) {}
    bool is_valid() const { return (id_invalid != value); }
    uint32_t as_uint32_t() const { return value; };

private:
    uint32_t value;
    static const uint32_t id_invalid = 0;
};

inline bool operator < (BufferID const& lhs, BufferID const& rhs)
{
    return lhs.as_uint32_t() < rhs.as_uint32_t();
}

inline bool operator == (BufferID const& lhs, BufferID const& rhs)
{
    return lhs.as_uint32_t() == rhs.as_uint32_t();
}
inline bool operator != (BufferID const& lhs, BufferID const& rhs)
{
    return lhs.as_uint32_t() != rhs.as_uint32_t();
}

}
}
#endif /* MIR_GRAPHICS_BUFFER_ID_H_ */
