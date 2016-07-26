/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/graphics/nested/buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/test/doubles/stub_host_connection.h"
#include "mir/test/fake_shared.h"
#include "mir_toolkit/client_types_nbs.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
using namespace testing;
namespace
{
struct MockHostConnection : mtd::StubHostConnection
{
    MOCK_METHOD1(create_buffer, std::shared_ptr<MirBuffer>(mg::BufferProperties const&));
};

struct NestedBuffer : Test
{
    MockHostConnection mock_connection;
    mg::BufferProperties properties;
    std::shared_ptr<MirBuffer> buffer;
};
}

TEST_F(NestedBuffer, creates_buffer)
{
    EXPECT_CALL(mock_connection, create_buffer(properties))
        .WillOnce(Return(buffer));
    mgn::Buffer buffer(mt::fake_shared(mock_connection), properties);
}
