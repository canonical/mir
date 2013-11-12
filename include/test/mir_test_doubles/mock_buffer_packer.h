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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_BUFFER_PACKER_H_
#define MIR_TEST_DOUBLES_MOCK_BUFFER_PACKER_H_

#include "mir/graphics/buffer_ipc_packer.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockPacker : public graphics::BufferIPCPacker
{
    ~MockPacker() noexcept {}
    MOCK_METHOD1(pack_fd, void(int));
    MOCK_METHOD1(pack_data, void(int));
    MOCK_METHOD1(pack_stride, void(geometry::Stride));
    MOCK_METHOD1(pack_flags, void(unsigned int));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_BUFFER_PACKER_H_ */
