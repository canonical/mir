/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_COOKIE_COOKIE_H_
#define MIR_COOKIE_COOKIE_H_

#include <stdint.h>
#include <vector>

namespace mir
{
namespace cookie
{

enum class Format : uint8_t
{
    HMAC_SHA_1_8
};

class MirCookie
{
public:
    virtual ~MirCookie() = default;

    /**
    *  Returns the timestamp that the cookie is built with
    *
    *  \return  The timestamp
    */
    virtual uint64_t timestamp() const = 0;

    /**
    *  Converts the cookie into a stream of bytes.
    *
    *  \return  The stream of bytes formated to be unmarshalled
    */
    virtual std::vector<uint8_t> marshall() const = 0;

    virtual bool operator==(MirCookie const& cookie) const = 0;
    virtual bool operator!=(MirCookie const& cookie) const = 0;
};

}
}

#endif // MIR_COOKIE_COOKIE_H_
