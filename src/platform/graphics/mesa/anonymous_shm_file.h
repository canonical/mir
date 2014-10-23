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

#ifndef MIR_GRAPHICS_MESA_ANONYMOUS_SHM_FILE_H_
#define MIR_GRAPHICS_MESA_ANONYMOUS_SHM_FILE_H_

#include "shm_file.h"
#include "mir/fd.h"

namespace mir
{
namespace graphics
{
namespace mesa
{

namespace detail
{
class MapHandle
{
public:
    MapHandle(int fd, size_t size);
    ~MapHandle() noexcept;

    operator void*() const;

private:
    MapHandle(MapHandle const&) = delete;
    MapHandle& operator=(MapHandle const&) = delete;
    size_t const size;
    void* const mapping;
};
}

class AnonymousShmFile : public ShmFile
{
public:
    AnonymousShmFile(size_t size);

    void* base_ptr() const;
    int fd() const;

private:
    Fd const fd_;
    detail::MapHandle const mapping;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_ANONYMOUS_SHM_FILE_H_ */
