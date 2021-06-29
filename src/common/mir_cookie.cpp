/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir_cookie.h"

#include "mir/require.h"

#include <string.h>

MirCookie::MirCookie(void const* buffer, size_t size) :
    cookie_(size)
{
    memcpy(cookie_.data(), buffer, size);
}

MirCookie::MirCookie(std::vector<uint8_t> const& cookie) :
    cookie_(cookie)
{
}

void MirCookie::copy_to(void* buffer, size_t size) const
{
    mir::require(size == cookie_.size());
    memcpy(buffer, cookie_.data(), size);
}

size_t MirCookie::size() const
{
    return cookie_.size();
}

std::vector<uint8_t> MirCookie::cookie() const
{
    return cookie_;
}
