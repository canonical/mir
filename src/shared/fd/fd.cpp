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

mir::Fd::RefcountFd::RefcountFd(int fd) :
    fd(fd),
    refcount(1)
{
}

mir::Fd::Fd(int other_fd) :
    rfd{new RefcountFd{other_fd}}
{
}

mir::Fd::Fd(Fd const& other) :
    rfd{other.rfd}
{
    if (rfd) rfd->refcount++;
}

mir::Fd::Fd(Fd&& other) :
    rfd{other.rfd}
{
    other.rfd = nullptr;
}

mir::Fd::~Fd() noexcept
{
    if (rfd)
    {
        rfd->refcount--;
        if ((rfd->fd > invalid) &&
            (rfd->refcount == 0))
        {
            ::close(rfd->fd);
            delete rfd;
        }
    }
}

mir::Fd& mir::Fd::operator=(Fd other)
{
    std::swap(rfd, other.rfd);
    return *this;
}

mir::Fd::operator int() const
{
    if (rfd)
        return rfd->fd;
    else
        return -1;
}
