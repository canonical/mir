/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"

#include "mir_test_framework/udev_environment.h"

#include "src/platforms/mesa/server/kms/platform.h"
#include "src/platforms/mesa/server/gbm_buffer.h"
#include "src/platforms/mesa/include/native_buffer.h"
#include "src/platforms/mesa/server/buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/test/doubles/null_emergency_cleanup.h"
#include "src/server/report/null_report_factory.h"
#include "mir/test/doubles/null_console_services.h"

#include <gbm.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mgm=mir::graphics::mesa;
namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;
namespace mtf=mir_test_framework;

class GBMBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        fake_devices.add_standard_device("standard-drm-devices");

        size = geom::Size{300, 200};
        pf = mir_pixel_format_argb_8888;
        stride = geom::Stride{4 * size.width.as_uint32_t()};
        usage = mg::BufferUsage::hardware;
        buffer_properties = mg::BufferProperties{size, pf, usage};

        ON_CALL(mock_gbm, gbm_bo_get_width(_))
        .WillByDefault(Return(size.width.as_uint32_t()));

        ON_CALL(mock_gbm, gbm_bo_get_height(_))
        .WillByDefault(Return(size.height.as_uint32_t()));

        ON_CALL(mock_gbm, gbm_bo_get_format(_))
        .WillByDefault(Return(GBM_BO_FORMAT_ARGB8888));

        ON_CALL(mock_gbm, gbm_bo_get_stride(_))
        .WillByDefault(Return(stride.as_uint32_t()));

        platform = std::make_shared<mgm::Platform>(
                mir::report::null_display_report(),
                std::make_shared<mtd::NullConsoleServices>(),
                *std::make_shared<mtd::NullEmergencyCleanup>(),
                mgm::BypassOption::allowed);

        allocator.reset(new mgm::BufferAllocator(platform->gbm->device, mgm::BypassOption::allowed, mgm::BufferImportMethod::gbm_native_pixmap));
    }

    mir::renderer::gl::TextureSource* as_texture_source(std::shared_ptr<mg::Buffer> const& buffer)
    {
        return dynamic_cast<mir::renderer::gl::TextureSource*>(buffer->native_buffer_base());
    }

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL>  mock_gl;
    std::shared_ptr<mgm::Platform> platform;
    std::unique_ptr<mgm::BufferAllocator> allocator;

    // Defaults
    MirPixelFormat pf;
    geom::Size size;
    geom::Stride stride;
    mg::BufferUsage usage;
    mg::BufferProperties buffer_properties;

    mtf::UdevEnvironment fake_devices;
};

TEST_F(GBMBufferTest, dimensions_test)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    auto buffer = allocator->alloc_buffer(buffer_properties);
    ASSERT_EQ(size, buffer->size());
}

TEST_F(GBMBufferTest, buffer_has_expected_pixel_format)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    auto buffer(allocator->alloc_buffer(buffer_properties));
    ASSERT_EQ(pf, buffer->pixel_format());
}

TEST_F(GBMBufferTest, stride_has_sane_value)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    // RGBA 8888 cannot take less than 4 bytes
    // TODO: is there a *maximum* sane value for stride?
    geom::Stride minimum(size.width.as_uint32_t() * 4);

    auto buffer(allocator->alloc_buffer(buffer_properties));

    auto native = std::dynamic_pointer_cast<mgm::NativeBuffer>(buffer->native_buffer_handle());
    ASSERT_THAT(native, Ne(nullptr));
    ASSERT_LE(minimum, geom::Stride{native->stride});
}

TEST_F(GBMBufferTest, buffer_native_handle_has_correct_size)
{
    using namespace testing;

    auto buffer = allocator->alloc_buffer(buffer_properties);
    auto native_handle = std::dynamic_pointer_cast<mgm::NativeBuffer>(buffer->native_buffer_handle());
    ASSERT_THAT(native_handle, Ne(nullptr));
    EXPECT_EQ(1, native_handle->fd_items);
    EXPECT_EQ(0, native_handle->data_items);
}

MATCHER_P(GEMFlinkHandleIs, value, "")
{
    auto flink = reinterpret_cast<struct drm_gem_flink*>(arg);
    return flink->handle == value;
}

ACTION_P(SetGEMFlinkName, value)
{
    auto flink = reinterpret_cast<struct drm_gem_flink*>(arg2);
    flink->name = value;
}

TEST_F(GBMBufferTest, buffer_native_handle_contains_correct_data)
{
    using namespace testing;

    uint32_t prime_fd{0x77};
    gbm_bo_handle mock_handle;
    mock_handle.u32 = 0xdeadbeef;

    EXPECT_CALL(mock_gbm, gbm_bo_get_handle(_))
            .Times(Exactly(1))
            .WillOnce(Return(mock_handle));

    EXPECT_CALL(mock_drm, drmPrimeHandleToFD(_,mock_handle.u32,_,_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<3>(prime_fd), Return(0)));

    auto buffer = allocator->alloc_buffer(buffer_properties);
    auto handle = std::dynamic_pointer_cast<mgm::NativeBuffer>(buffer->native_buffer_handle());
    ASSERT_THAT(handle, Ne(nullptr));
    EXPECT_EQ(prime_fd, static_cast<unsigned int>(handle->fd[0]));
    EXPECT_EQ(stride.as_uint32_t(), static_cast<unsigned int>(handle->stride));
}

TEST_F(GBMBufferTest, buffer_creation_throws_on_prime_fd_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_drm, drmPrimeHandleToFD(_,_,_,_))
            .Times(Exactly(1))
            .WillOnce(Return(-1));

    EXPECT_THROW({
        auto buffer = allocator->alloc_buffer(buffer_properties);
    }, std::runtime_error);
}

TEST_F(GBMBufferTest, gl_bind_to_texture_egl_image_creation_failed)
{
    using namespace testing;

    ON_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .WillByDefault(Return(EGL_NO_IMAGE_KHR));

    EXPECT_THROW({
        auto buffer = allocator->alloc_buffer(buffer_properties);
        as_texture_source(buffer)->gl_bind_to_texture();
    }, std::runtime_error);
}

TEST_F(GBMBufferTest, gl_bind_to_texture_uses_egl_image)
{
    using namespace testing;

    {
        InSequence seq;

        EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
            .Times(Exactly(1));

        EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(_,mock_egl.fake_egl_image))
            .Times(Exactly(1));

        EXPECT_CALL(mock_egl, eglDestroyImageKHR(_,mock_egl.fake_egl_image))
            .Times(Exactly(1));
    }

    EXPECT_NO_THROW({
        auto buffer = allocator->alloc_buffer(buffer_properties);
        as_texture_source(buffer)->gl_bind_to_texture();
    });
}
