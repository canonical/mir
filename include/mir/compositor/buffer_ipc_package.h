/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_IPC_PACKAGE_H_
#define MIR_COMPOSITOR_BUFFER_IPC_PACKAGE_H_

#include <vector>

namespace mir
{
namespace compositor
{

class BufferIPCPackage
{
public:
    virtual ~BufferIPCPackage() {}

    virtual std::vector<int> get_ipc_data() = 0;
    virtual std::vector<int> get_ipc_fds() = 0;

    BufferIPCPackage(BufferIPCPackage const&) = delete;
    BufferIPCPackage& operator=(BufferIPCPackage const&) = delete;

protected:
    BufferIPCPackage() {}

};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_IPC_PACKAGE_H_ */
