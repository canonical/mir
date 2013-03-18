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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_BUFFER_H_

#include "mir/compositor/buffer_basic.h"
#include "mir/geometry/size.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/buffer_id.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockBuffer : public compositor::Buffer
{
 public:
    MockBuffer(geometry::Size size,
               geometry::Stride s,
               geometry::PixelFormat pf)
    {
        empty_package = std::make_shared<compositor::BufferIPCPackage>();

        using namespace testing;
        ON_CALL(*this, size())
                .WillByDefault(Return(size));
        ON_CALL(*this, stride())
                .WillByDefault(Return(s));
        ON_CALL(*this, pixel_format())
                .WillByDefault(Return(pf));

        ON_CALL(*this, get_ipc_package())
                .WillByDefault(Return(empty_package));
        ON_CALL(*this, id())
                .WillByDefault(Return(compositor::BufferID{4}));
    }

    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(stride, geometry::Stride());
    MOCK_CONST_METHOD0(pixel_format, geometry::PixelFormat());
    MOCK_CONST_METHOD0(get_ipc_package, std::shared_ptr<compositor::BufferIPCPackage>());

    MOCK_METHOD0(bind_to_texture, void());
    MOCK_CONST_METHOD0(id, compositor::BufferID());

    std::shared_ptr<compositor::BufferIPCPackage> empty_package;
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_BUFFER_H_
