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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test/mock_buffer.h"

#include "mir/geometry/dimensions.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_swapper_double.h"

#include <mir_test/gmock_fixes.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{
const geom::Width width {1024};
const geom::Height height {768};
const geom::Stride stride {geom::dim_cast<geom::Stride>(width)};
const mc::PixelFormat pixel_format {mc::PixelFormat::rgba_8888};

struct MockSwapper : public mc::BufferSwapper
{
public:
    MockSwapper() {};
    MockSwapper(std::shared_ptr<mc::Buffer> buffer)
        : default_buffer(buffer)
    {
        using namespace testing;

        ON_CALL(*this, compositor_acquire())
        .WillByDefault(Return(default_buffer.get()));
        ON_CALL(*this, client_acquire())
        .WillByDefault(Return(default_buffer.get()));
    };

    MOCK_METHOD0(client_acquire,   mc::Buffer*(void));
    MOCK_METHOD1(client_release, void(mc::Buffer*));
    MOCK_METHOD0(compositor_acquire,  mc::Buffer*(void));
    MOCK_METHOD1(compositor_release,   void(mc::Buffer*));

private:
    std::shared_ptr<mc::Buffer> default_buffer;
};
}


TEST(buffer_bundle, get_buffer_for_compositor)
{
    using namespace testing;
    std::shared_ptr<mc::MockBuffer> mock_buffer(new mc::MockBuffer {width, height, stride, pixel_format});
    std::unique_ptr<MockSwapper> mock_swapper(new MockSwapper(mock_buffer));

    EXPECT_CALL(*mock_buffer, bind_to_texture())
    .Times(1);

    EXPECT_CALL(*mock_swapper, compositor_acquire())
    .Times(1);

    EXPECT_CALL(*mock_swapper, compositor_release(_));

    mc::BufferBundle buffer_bundle(std::move(mock_swapper));

    /* if binding doesn't work, this is a case where we may have an exception */
    ASSERT_NO_THROW(
    {
        auto texture = buffer_bundle.lock_and_bind_back_buffer();
    });
}

TEST(buffer_bundle, get_buffer_for_client)
{
    using namespace testing;
    std::shared_ptr<mc::MockBuffer> mock_buffer(new mc::MockBuffer {width, height, stride, pixel_format});
    std::unique_ptr<MockSwapper> mock_swapper(new MockSwapper(mock_buffer));

    EXPECT_CALL(*mock_buffer, lock())
    .Times(1);

    EXPECT_CALL(*mock_swapper, client_acquire())
    .Times(1);

    EXPECT_CALL(*mock_swapper, client_release(_));

    mc::BufferBundle buffer_bundle(std::move(mock_swapper));

    /* if dequeue doesn't work, this is a case where we may have an exception */
    ASSERT_NO_THROW(
    {
        auto buffer_package = buffer_bundle.secure_client_buffer();
    });
}
