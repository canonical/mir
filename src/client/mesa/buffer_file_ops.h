/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_MESA_BUFFER_FILE_OPS_
#define MIR_CLIENT_MESA_BUFFER_FILE_OPS_

#include <sys/types.h>

namespace mir
{
namespace client
{
namespace mesa
{

class BufferFileOps
{
public:
    virtual ~BufferFileOps() = default;

    virtual int close(int fd) const = 0;
    virtual void* map(int fd, off_t offset, size_t size) const = 0;
    virtual void unmap(void* addr, size_t size) const = 0;

protected:
    BufferFileOps() = default;
    BufferFileOps(BufferFileOps const&) = delete;
    BufferFileOps& operator=(BufferFileOps const&) = delete;
};

}
}
}

#endif /* MIR_CLIENT_MESA_BUFFER_FILE_OPS_ */
