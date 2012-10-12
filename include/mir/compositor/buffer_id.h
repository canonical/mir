/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_COMPOSITOR_BUFFER_ID_H_
#define MIR_COMPOSITOR_BUFFER_ID_H_

#include <cstdint>
namespace mir
{
namespace compositor
{

class BufferID
{
public:
    BufferID() : value(id_invalid){}
    BufferID(int val) : value(val){}
    bool is_valid() { return false; }
    uint32_t as_uint32_t() const { return value; };

private:
    const uint32_t value;
    static const int id_invalid = 0;
};

inline bool operator == (BufferID const& /*lhs*/, BufferID const& /*rhs*/)
{
//    return lhs.as_uint32_t() == rhs.as_uint32_t();
    return false;
}
inline bool operator != (BufferID const& /*lhs*/, BufferID const& /*rhs*/)
{
//    return lhs.as_uint32_t() == rhs.as_uint32_t();
    return false;
}

}
}
#endif /* MIR_COMPOSITOR_BUFFER_ID_H_ */
