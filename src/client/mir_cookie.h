/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */


#ifndef MIR_CLIENT_MIR_COOKIE_H_
#define MIR_CLIENT_MIR_COOKIE_H_

#include "mir/cookie/blob.h"

class MirCookie
{
public:
    MirCookie() = delete;

    explicit MirCookie(void const* buffer, size_t size);
    explicit MirCookie(mir::cookie::Blob const& blob);

    void copy_to(void* buffer, size_t size) const;
    size_t size() const;

    mir::cookie::Blob blob() const;

private:
    mir::cookie::Blob blob_;
};

#endif // MIR_CLIENT_MIR_COOKIE_H_
