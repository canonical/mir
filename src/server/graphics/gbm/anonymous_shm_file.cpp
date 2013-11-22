/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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

namespace mgg = mir::graphics::gbm;

namespace
{

int create_anonymous_file(size_t size)
{
    char const* const tmpl = "/mir-buffer-XXXXXX";
    char const* const runtime_dir = getenv("XDG_RUNTIME_DIR");
    char const* const target_dir = runtime_dir ? runtime_dir : "/tmp";
    int fd = -1;

    /* We need a mutable array for mkostemp */
    std::vector<char> path(target_dir, target_dir + strlen(target_dir));
    path.insert(path.end(), tmpl, tmpl + strlen(tmpl));
    path.push_back('\0');

    try
    {
        /* TODO: RAII fd */
        fd = mkostemp(path.data(), O_CLOEXEC);
        if (fd < 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create temporary file"));
        if (unlink(path.data()) < 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to unlink temporary file"));
        if (ftruncate(fd, size) < 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to resize temporary file"));
    }
    catch (...)
    {
        if (fd >= 0)
            close(fd);
        throw;
    }

    return fd;
}

}

mgg::AnonymousShmFile::AnonymousShmFile(size_t size)
    : fd_{create_anonymous_file(size)},
      size{size},
      mapping{MAP_FAILED}
{
}

mgg::AnonymousShmFile::~AnonymousShmFile() noexcept
{
    if (mapping != MAP_FAILED)
        munmap(mapping, size);
    close(fd_);
}

void* mgg::AnonymousShmFile::map()
{
    if (mapping == MAP_FAILED)
    {
        mapping = mmap(nullptr, size, PROT_READ|PROT_WRITE,
                       MAP_SHARED, fd_, 0);
    }

    return mapping;
}

int mgg::AnonymousShmFile::fd() const
{
    return fd_;
}
