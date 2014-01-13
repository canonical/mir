/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by:
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "anonymous_shm_file.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <vector>

#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>

namespace mgm = mir::graphics::mesa;

namespace
{

mgm::detail::FdHandle create_anonymous_file(size_t size)
{
    char const* const tmpl = "/mir-buffer-XXXXXX";
    char const* const runtime_dir = getenv("XDG_RUNTIME_DIR");
    char const* const target_dir = runtime_dir ? runtime_dir : "/tmp";

    /* We need a mutable array for mkostemp */
    std::vector<char> path(target_dir, target_dir + strlen(target_dir));
    path.insert(path.end(), tmpl, tmpl + strlen(tmpl));
    path.push_back('\0');

    mgm::detail::FdHandle fd{mkostemp(path.data(), O_CLOEXEC)};
    if (unlink(path.data()) < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to unlink temporary file"));
    if (ftruncate(fd, size) < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to resize temporary file"));

    return fd;
}

}

/*************
 * FdHandle *
 *************/

mgm::detail::FdHandle::FdHandle(int fd)
    : fd{fd}
{
    if (fd < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create file"));
}

mgm::detail::FdHandle::FdHandle(FdHandle&& other)
    : fd{other.fd}
{
    other.fd = -1;
}

mgm::detail::FdHandle::~FdHandle() noexcept
{
    if (fd >= 0)
        close(fd);
}

mgm::detail::FdHandle::operator int() const
{
    return fd;
}

/*************
 * MapHandle *
 *************/

mgm::detail::MapHandle::MapHandle(int fd, size_t size)
    : size{size},
      mapping{mmap(nullptr, size, PROT_READ|PROT_WRITE,
                   MAP_SHARED, fd, 0)}
{
    if (mapping == MAP_FAILED)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to map file"));
}

mgm::detail::MapHandle::~MapHandle() noexcept
{
    munmap(mapping, size);
}

mgm::detail::MapHandle::operator void*() const
{
    return mapping;

}

/********************
 * AnonymousShmFile *
 ********************/

mgm::AnonymousShmFile::AnonymousShmFile(size_t size)
    : fd_{create_anonymous_file(size)},
      mapping{fd_, size}
{
}

void* mgm::AnonymousShmFile::base_ptr() const
{
    return mapping;
}

int mgm::AnonymousShmFile::fd() const
{
    return fd_;
}
