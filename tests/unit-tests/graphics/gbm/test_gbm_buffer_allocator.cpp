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

#include "src/server/graphics/gbm/gbm_platform.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "src/server/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"

#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#include "mir_test_doubles/mock_buffer_initializer.h"
#include "mir_test_doubles/null_virtual_terminal.h"
#include "mir_test_framework/udev_environment.h"
#include "mir/graphics/null_display_report.h"

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gbm.h>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mtf = mir::mir_test_framework;

class GBMBufferAllocatorTest  : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        unsetenv("MIR_BYPASS");

        fake_devices.add_standard_drm_devices();

        size = geom::Size{300, 200};
        pf = geom::PixelFormat::argb_8888;
        usage = mg::BufferUsage::hardware;
        buffer_properties = mg::BufferProperties{size, pf, usage};

        ON_CALL(mock_gbm, gbm_bo_get_handle(_))
        .WillByDefault(Return(mock_gbm.fake_gbm.bo_handle));

        platform = std::make_shared<mgg::GBMPlatform>(std::make_shared<mg::NullDisplayReport>(),
                                                      std::make_shared<mtd::NullVirtualTerminal>());
        mock_buffer_initializer = std::make_shared<testing::NiceMock<mtd::MockBufferInitializer>>();
        allocator.reset(new mgg::GBMBufferAllocator(platform->gbm.device, mock_buffer_initializer));
    }

    // Defaults
    geom::Size size;
    geom::PixelFormat pf;
    mg::BufferUsage usage;
    mg::BufferProperties buffer_properties;

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
    std::shared_ptr<mgg::GBMPlatform> platform;
    std::shared_ptr<testing::NiceMock<mtd::MockBufferInitializer>> mock_buffer_initializer;
    std::unique_ptr<mgg::GBMBufferAllocator> allocator;
    mtf::UdevEnvironment fake_devices;
};

TEST_F(GBMBufferAllocatorTest, allocator_returns_non_null_buffer)
{
    using namespace testing;
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    EXPECT_TRUE(allocator->alloc_buffer(buffer_properties).get() != NULL);
}

TEST_F(GBMBufferAllocatorTest, large_hardware_buffers_bypass)
{
    using namespace testing;
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    const mg::BufferProperties properties(geom::Size{1280, 800},
                                          geom::PixelFormat::argb_8888,
                                          mg::BufferUsage::hardware);

    auto buf = allocator->alloc_buffer(properties);
    ASSERT_TRUE(buf.get() != NULL);
    EXPECT_TRUE(buf->can_bypass());
}

TEST_F(GBMBufferAllocatorTest, small_buffers_dont_bypass)
{
    using namespace testing;
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    const mg::BufferProperties properties(geom::Size{100, 100},
                                          geom::PixelFormat::argb_8888,
                                          mg::BufferUsage::hardware);

    auto buf = allocator->alloc_buffer(properties);
    ASSERT_TRUE(buf.get() != NULL);
    EXPECT_FALSE(buf->can_bypass());
}

TEST_F(GBMBufferAllocatorTest, software_buffers_dont_bypass)
{
    using namespace testing;
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    const mg::BufferProperties properties(geom::Size{1920, 1200},
                                          geom::PixelFormat::argb_8888,
                                          mg::BufferUsage::software);

    auto buf = allocator->alloc_buffer(properties);
    ASSERT_TRUE(buf.get() != NULL);
    EXPECT_FALSE(buf->can_bypass());
}

TEST_F(GBMBufferAllocatorTest, bypass_disables_via_environment)
{
    using namespace testing;
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    const mg::BufferProperties properties(geom::Size{1280, 800},
                                          geom::PixelFormat::argb_8888,
                                          mg::BufferUsage::hardware);

    setenv("MIR_BYPASS", "0", 1);
    mgg::GBMBufferAllocator alloc(platform->gbm.device,
                                  mock_buffer_initializer);
    auto buf = alloc.alloc_buffer(properties);
    ASSERT_TRUE(buf.get() != NULL);
    EXPECT_FALSE(buf->can_bypass());
    unsetenv("MIR_BYPASS");
}

TEST_F(GBMBufferAllocatorTest, correct_buffer_format_translation_argb_8888)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,GBM_FORMAT_ARGB8888,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(mg::BufferProperties{size, geom::PixelFormat::argb_8888, usage});
}

TEST_F(GBMBufferAllocatorTest, correct_buffer_format_translation_xrgb_8888)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,GBM_FORMAT_XRGB8888,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(mg::BufferProperties{size, geom::PixelFormat::xrgb_8888, usage});
}

MATCHER_P(has_flag_set, flag, "")
{
    return arg & flag;
}

TEST_F(GBMBufferAllocatorTest, creates_hardware_rendering_buffer)
{
    using namespace testing;

    mg::BufferProperties properties{size, pf, mg::BufferUsage::hardware};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,has_flag_set(GBM_BO_USE_RENDERING)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(properties);
}

TEST_F(GBMBufferAllocatorTest, creates_software_rendering_buffer)
{
    using namespace testing;

    mg::BufferProperties properties{size, pf, mg::BufferUsage::software};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,has_flag_set(GBM_BO_USE_WRITE|GBM_BO_USE_RENDERING)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(properties);
}

TEST_F(GBMBufferAllocatorTest, creates_hardware_rendering_buffer_for_undefined_usage)
{
    using namespace testing;

    mg::BufferProperties properties{size, pf, mg::BufferUsage::undefined};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,has_flag_set(GBM_BO_USE_RENDERING)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(properties);
}

TEST_F(GBMBufferAllocatorTest, requests_correct_buffer_dimensions)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,size.width.as_uint32_t(),size.height.as_uint32_t(),_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(buffer_properties);
}

TEST_F(GBMBufferAllocatorTest, correct_buffer_handle_is_destroyed)
{
    using namespace testing;
    gbm_bo* bo{reinterpret_cast<gbm_bo*>(0xabcd)};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_))
    .WillOnce(Return(bo));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(bo));

    allocator->alloc_buffer(buffer_properties);
}

TEST_F(GBMBufferAllocatorTest, buffer_initializer_is_called)
{
    using namespace testing;

    EXPECT_CALL(*mock_buffer_initializer, operator_call(_))
        .Times(1);

    allocator->alloc_buffer(buffer_properties);
}

TEST_F(GBMBufferAllocatorTest, null_buffer_initializer_does_not_crash)
{
    using namespace testing;

    auto null_buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    allocator.reset(new mgg::GBMBufferAllocator(platform->gbm.device, null_buffer_initializer));

    EXPECT_NO_THROW({
        allocator->alloc_buffer(buffer_properties);
    });
}

TEST_F(GBMBufferAllocatorTest, throws_on_buffer_creation_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_))
        .WillOnce(Return(reinterpret_cast<gbm_bo*>(0)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_))
        .Times(0);

    EXPECT_THROW({
        allocator->alloc_buffer(buffer_properties);
    }, std::runtime_error);
}

TEST_F(GBMBufferAllocatorTest, supported_pixel_formats_contain_common_formats)
{
    auto supported_pixel_formats = allocator->supported_pixel_formats();

    auto argb_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      geom::PixelFormat::argb_8888);

    auto xrgb_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      geom::PixelFormat::xrgb_8888);

    EXPECT_EQ(1, argb_8888_count);
    EXPECT_EQ(1, xrgb_8888_count);
}

TEST_F(GBMBufferAllocatorTest, supported_pixel_formats_have_sane_default_in_first_position)
{
    auto supported_pixel_formats = allocator->supported_pixel_formats();

    ASSERT_FALSE(supported_pixel_formats.empty());
    EXPECT_EQ(geom::PixelFormat::argb_8888, supported_pixel_formats[0]);
}

TEST_F(GBMBufferAllocatorTest, alloc_with_unsupported_pixel_format_throws)
{
    using namespace testing;

    /* We shouldn't try to create a buffer with an unsupported format */
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_)).Times(0);

    EXPECT_THROW({
        allocator->alloc_buffer(mg::BufferProperties{size, geom::PixelFormat::abgr_8888, usage});
    }, std::runtime_error);
}
