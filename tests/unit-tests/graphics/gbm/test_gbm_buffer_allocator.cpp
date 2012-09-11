
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/graphics/gbm/gbm_buffer_allocator.h"

#include "mock_gbm.h"

#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gbm.h>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{
struct GBMDeviceDeleter
{
    void operator()(gbm_device* d) const
    {            
        if (d)
            gbm_device_destroy(d);
    }
};
}

class GBMBufferAllocatorTest  : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        std::shared_ptr<gbm_device> fake_device{gbm_create_device(1), GBMDeviceDeleter()};
        allocator.reset(new mgg::GBMBufferAllocator(fake_device));

        w = geom::Width(300);
        h = geom::Height(200);
        pf = mc::PixelFormat::rgba_8888;
    }

    // Defaults
    geom::Width w;
    geom::Height h;
    mc::PixelFormat pf;

    ::testing::NiceMock<mgg::MockGBM> mock_gbm;
    std::unique_ptr<mgg::GBMBufferAllocator> allocator;
};

TEST_F(GBMBufferAllocatorTest, allocator_returns_non_null_buffer)
{
    using namespace testing;
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    EXPECT_TRUE(allocator->alloc_buffer(w, h, pf).get() != NULL);
}

TEST_F(GBMBufferAllocatorTest, correct_buffer_format_translation)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,GBM_BO_FORMAT_ARGB8888,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(w, h, mc::PixelFormat::rgba_8888);
}

static bool has_hardware_rendering_flag_set(uint32_t flags) {
    return flags & GBM_BO_USE_RENDERING;
}

TEST_F(GBMBufferAllocatorTest, creates_hw_rendering_buffer_by_default)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,Truly(has_hardware_rendering_flag_set)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(w, h, pf);
}

TEST_F(GBMBufferAllocatorTest, requests_correct_buffer_dimensions)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,w.as_uint32_t(),h.as_uint32_t(),_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(w, h, pf);
}

TEST_F(GBMBufferAllocatorTest, correct_buffer_handle_is_destroyed)
{
    using namespace testing;
    gbm_bo* bo{reinterpret_cast<gbm_bo*>(0xabcd)};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_))
    .WillOnce(Return(bo));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(bo));

    allocator->alloc_buffer(w, h, pf);
}
