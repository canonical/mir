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
#include <boost/filesystem.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <stdexcept>

#include <vector>

#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>

namespace mgc = mir::graphics::common;

namespace
{

mir::Fd create_anonymous_file(size_t size)
{
    auto raw_fd = open("/dev/shm", O_TMPFILE | O_RDWR | O_EXCL, S_IRWXU);

    // Workaround for filesystems that don't support O_TMPFILE
    if (raw_fd == -1 && errno == EINVAL)
    {
        char template_filename[] = "/dev/shm/mir-buffer-XXXXXX";
        raw_fd = mkostemp(template_filename, O_CLOEXEC);
        if (raw_fd != -1)
        {
            if (unlink(template_filename) < 0)
            {
                close(raw_fd);
                raw_fd = -1;
            }
        }
    }

    if (raw_fd == -1)
        BOOST_THROW_EXCEPTION(boost::enable_error_info(
            std::runtime_error("Failed to open temporary file"))
               << boost::errinfo_errno(errno));

    mir::Fd fd = mir::Fd{raw_fd};

    if (ftruncate(fd, size) == -1)
        BOOST_THROW_EXCEPTION(boost::enable_error_info(
            std::runtime_error("Failed to resize temporary file"))
                << boost::errinfo_errno(errno));

    return fd;
}

}

/*************
 * MapHandle *
 *************/

mgc::detail::MapHandle::MapHandle(int fd, size_t size)
    : size{size},
      mapping{mmap(nullptr, size, PROT_READ|PROT_WRITE,
                   MAP_SHARED, fd, 0)}
{
    if (mapping == MAP_FAILED)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to map file"));
}

mgc::detail::MapHandle::~MapHandle() noexcept
{
    munmap(mapping, size);
}

mgc::detail::MapHandle::operator void*() const
{
    return mapping;

}

/********************
 * AnonymousShmFile *
 ********************/

mgc::AnonymousShmFile::AnonymousShmFile(size_t size)
    : fd_{create_anonymous_file(size)},
      mapping{fd_, size}
{
}

void* mgc::AnonymousShmFile::base_ptr() const
{
    return mapping;
}

int mgc::AnonymousShmFile::fd() const
{
    return fd_;
}
