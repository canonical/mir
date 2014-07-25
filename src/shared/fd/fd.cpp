/*
 * Copyright © 2014 Canonical Ltd.
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
#include "mir/fd.h"
#include <unistd.h>
#include <algorithm>

mir::Fd::Fd() :
    Fd{invalid}
{
}

mir::Fd::CountedFd::CountedFd(int raw_fd) :
    raw_fd(raw_fd),
    refcount(1)
{
}

mir::Fd::Fd(int other_fd) :
    fd{new CountedFd{other_fd}}
{
}

mir::Fd::Fd(Fd const& other) :
    fd{other.fd}
{
    if (fd) fd->refcount++;
}

mir::Fd::Fd(Fd&& other) :
    fd{other.fd}
{
    other.fd = nullptr;
}

mir::Fd::~Fd() noexcept
{
    if ((fd) && (--fd->refcount == 0))
    {
        if (fd->raw_fd > invalid) ::close(fd->raw_fd);
        delete fd;
    }
}

mir::Fd& mir::Fd::operator=(Fd other)
{
    std::swap(fd, other.fd);
    return *this;
}

mir::Fd::operator int() const
{
    if (fd) return fd->raw_fd;
    return invalid;
}
