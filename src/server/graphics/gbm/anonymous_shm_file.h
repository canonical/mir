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

#ifndef MIR_GRAPHICS_GBM_ANONYMOUS_SHM_FILE_H_
#define MIR_GRAPHICS_GBM_ANONYMOUS_SHM_FILE_H_

#include "shm_file.h"

namespace mir
{
namespace graphics
{
namespace gbm
{

class AnonymousShmFile : public ShmFile
{
public:
    AnonymousShmFile(size_t size);
    ~AnonymousShmFile() noexcept;

    void* map();
    int fd() const;

private:
    int const fd_;
    size_t const size;
    void* mapping;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_ANONYMOUS_SHM_FILE_H_ */
