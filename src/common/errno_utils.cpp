/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/errno_utils.h>

#include <cstring>

namespace
{
// Since we are normalizing between GNU/POSIX impl, one of these functions will be unused at compile time

[[maybe_unused]]
inline auto strerror_r_to_cstr(int ret, const char* buf) noexcept
{
    // POSIX
    return ret ? "Unknown error" : buf;
}

[[maybe_unused]]
inline auto strerror_r_to_cstr(char* ret, const char* /*buf*/) noexcept
{
    // GNU
    return ret ? ret : "Unknown error";
}

}

auto mir::errno_to_cstr(int err) noexcept -> const char*
{
    thread_local char buf[256] = {};

    auto ret = ::strerror_r(err, buf, sizeof(buf));
    return strerror_r_to_cstr(ret, buf);
}
