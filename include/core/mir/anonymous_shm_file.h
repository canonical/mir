/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by:
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CORE_ANONYMOUS_SHM_FILE_H_
#define MIR_CORE_ANONYMOUS_SHM_FILE_H_

#include "shm_file.h"
#include "mir/fd.h"

namespace mir
{

namespace detail
{
}

class AnonymousShmFile : public ShmFile
{
public:
    AnonymousShmFile(size_t size);
    ~AnonymousShmFile() noexcept;

    void* base_ptr() const override;
    int fd() const override;

private:
    Fd const fd_;
    class MapHandle;
    std::unique_ptr<MapHandle> const mapping;
};

}

#endif /* MIR_CORE_ANONYMOUS_SHM_FILE_H_ */
