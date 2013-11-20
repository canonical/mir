/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/surfaces/threaded_snapshot_strategy.h"
#include "src/server/surfaces/pixel_buffer.h"
#include "mir/shell/surface_buffer_access.h"
#include "mir/graphics/buffer.h"

#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <atomic>

namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

class StubSurfaceBufferAccess : public msh::SurfaceBufferAccess
{
public:
    ~StubSurfaceBufferAccess() noexcept {}

    void with_most_recent_buffer_do(
        std::function<void(mg::Buffer&)> const& exec)
    {
        exec(buffer);
    }

    mtd::StubBuffer buffer;
};

class MockPixelBuffer : public ms::PixelBuffer
{
public:
    ~MockPixelBuffer() noexcept {}

    MOCK_METHOD1(fill_from, void(mg::Buffer& buffer));
    MOCK_METHOD0(as_argb_8888, void const*());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
};

TEST(ThreadedSnapshotStrategyTest, takes_snapshot)
{
    using namespace testing;

    void const* pixels{reinterpret_cast<void*>(0xabcd)};
    geom::Size size{10, 11};
    geom::Stride stride{123};

    MockPixelBuffer pixel_buffer;
    StubSurfaceBufferAccess buffer_access;

    EXPECT_CALL(pixel_buffer, fill_from(Ref(buffer_access.buffer)));
    EXPECT_CALL(pixel_buffer, as_argb_8888())
        .WillOnce(Return(pixels));
    EXPECT_CALL(pixel_buffer, size())
        .WillOnce(Return(size));
    EXPECT_CALL(pixel_buffer, stride())
        .WillOnce(Return(stride));

    ms::ThreadedSnapshotStrategy strategy{mt::fake_shared(pixel_buffer)};

    std::atomic<bool> snapshot_taken{false};

    msh::Snapshot snapshot;

    strategy.take_snapshot_of(
        mt::fake_shared(buffer_access),
        [&](msh::Snapshot const& s)
        { 
            snapshot = s;
            snapshot_taken = true; 
        });

    while (!snapshot_taken)
        std::this_thread::sleep_for(std::chrono::milliseconds{1});

    EXPECT_EQ(size,   snapshot.size);
    EXPECT_EQ(stride, snapshot.stride);
    EXPECT_EQ(pixels, snapshot.pixels);
}
