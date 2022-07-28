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
 */

#ifndef MIR_CORE_SHM_FILE_H_
#define MIR_CORE_SHM_FILE_H_

#include <cstddef>

namespace mir
{
/// \remark Since MirCore 1.0
class ShmFile
{
public:
    virtual ~ShmFile() = default;

    virtual void* base_ptr() const = 0;
    virtual int fd() const = 0;

protected:
    ShmFile() = default;
    ShmFile(ShmFile const&) = delete;
    ShmFile& operator=(ShmFile const&) = delete;
};

}

#endif /* MIR_CORE_SHM_FILE_H_ */
