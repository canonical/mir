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
#undef _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L

#include <mir/errno_utils.h>

#include <cstring>

auto mir::errno_to_cstr(int err) noexcept -> const char*
{
    thread_local char buf[256] = {};
    return ::strerror_r(err, buf, sizeof(buf)) ? "Unknown error" : buf;
}
