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

#include "mir/graphics/android/android_alloc_adaptor.h"

#include <hardware/hardware.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{

class ICSAllocInterface
{
public:
    virtual int alloc_interface(alloc_device_t* dev, int w, int h,
                                int format, int usage, buffer_handle_t* handle, int* stride) = 0;
    virtual int free_interface(alloc_device_t* dev, buffer_handle_t handle) = 0;
    virtual int dump_interface(alloc_device_t* dev, char *buf, int len) = 0;

};

class MockAllocDevice : public ICSAllocInterface,
    public alloc_device_t
{
public:

    MockAllocDevice(buffer_handle_t handle)
        :
        buffer_handle(handle)
    {
        using namespace testing;

        alloc = hook_alloc;
        free = hook_free;
        dump = hook_dump;
        ON_CALL(*this, alloc_interface(_,_,_,_,_,_,_))
        .WillByDefault(DoAll(
                           SetArgPointee<5>(buffer_handle),
                           SetArgPointee<6>(300),
                           Return(0)));
        ON_CALL(*this, free_interface(_,_))
        .WillByDefault(Return(0));

    }

    static int hook_alloc(alloc_device_t* mock_alloc,
                          int w, int h, int format, int usage,
                          buffer_handle_t* handle, int* stride)
    {
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        return mocker->alloc_interface(mock_alloc, w, h, format, usage, handle, stride);
    }

    static int hook_free(alloc_device_t* mock_alloc, buffer_handle_t handle)
    {
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        return mocker->free_interface(mock_alloc, handle);
    }

    static void hook_dump(alloc_device_t* mock_alloc, char* buf, int buf_len)
    {
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        mocker->dump_interface(mock_alloc, buf, buf_len);
    }

    MOCK_METHOD7(alloc_interface,  int(alloc_device_t*, int, int, int, int, buffer_handle_t*, int*));
    MOCK_METHOD2(free_interface, int(alloc_device_t*, buffer_handle_t));
    MOCK_METHOD3(dump_interface, int(alloc_device_t*, char*, int));

    buffer_handle_t buffer_handle;
};
}

class AdaptorICSTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_alloc_device = std::shared_ptr<MockAllocDevice> (new MockAllocDevice(&native_handle));

        alloc_adaptor = std::shared_ptr<mga::AndroidAllocAdaptor> (new mga::AndroidAllocAdaptor(mock_alloc_device));

        /* set up common defaults */
        pf = mc::PixelFormat::rgba_8888;
        width = geom::Width(300);
        height = geom::Height(200);
        usage = mg::BufferUsage::use_hardware;

        stride = geom::Stride(300*4);

    }

    native_handle_t native_handle;
    std::shared_ptr<MockAllocDevice> mock_alloc_device;
    std::shared_ptr<mga::AndroidAllocAdaptor> alloc_adaptor;

    mc::PixelFormat pf;
    geom::Width width;
    geom::Height height;
    geom::Stride stride;
    std::shared_ptr<mg::BufferHandle> buffer_data;
    mg::BufferUsage usage;
};

TEST_F(AdaptorICSTest, resource_type_test_fail_ret)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, _, _, _, _, _, _))
    .WillOnce(DoAll(
                  SetArgPointee<5>((native_handle_t*) &native_handle),
                  SetArgPointee<6>(width.as_uint32_t()*4),
                  Return(-1)));

    buffer_data = alloc_adaptor->alloc_buffer(width, height, pf, usage );
    EXPECT_EQ(buffer_data.get(), (mg::BufferHandle*) NULL);
}

TEST_F(AdaptorICSTest, resource_type_test_fail_stride)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, _, _, _, _, _, _))
    .WillOnce(DoAll(
                  SetArgPointee<5>((native_handle_t*) &native_handle),
                  SetArgPointee<6>(0),
                  Return(0)));

    buffer_data = alloc_adaptor->alloc_buffer(width, height, pf, usage );
    EXPECT_EQ(buffer_data.get(), (mg::BufferHandle*) NULL);
}

TEST_F(AdaptorICSTest, resource_type_test_fail_null_handle)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, _, _, _, _, _, _))
    .WillOnce(DoAll(
                  SetArgPointee<5>((native_handle_t*)NULL),
                  SetArgPointee<6>(width.as_uint32_t()*4),
                  Return(0)));

    buffer_data = alloc_adaptor->alloc_buffer(width, height, pf, usage );
    EXPECT_EQ(buffer_data.get(), (mg::BufferHandle*) NULL);
}

TEST_F(AdaptorICSTest, resource_type_test_success_ret)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, _, _, _, _, _, _));
    EXPECT_CALL(*mock_alloc_device, free_interface( _, _) );

    buffer_data = alloc_adaptor->alloc_buffer(width, height, pf, usage );
    EXPECT_NE(buffer_data.get(), (mg::BufferHandle*) NULL);
}

#if 0
TEST_F(AdaptorICSTest, resource_type_test_success_stride_is_set)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, _, _, _, _, _, _));
    EXPECT_CALL(*mock_alloc_device, free_interface( _, _) );

    geom::Stride saved_stride(0);
    stride = geom::Stride(0);
    alloc_adaptor->alloc_buffer(buffer_data, stride, width, height, pf, usage );
    EXPECT_NE(saved_stride, stride );
}
#endif

TEST_F(AdaptorICSTest, resource_type_test_proper_alloc_is_used)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_interface( mock_alloc_device.get(), _, _, _, _, _, _));
    EXPECT_CALL(*mock_alloc_device, free_interface( mock_alloc_device.get(), _) );

    alloc_adaptor->alloc_buffer(width, height, pf, usage );
}

TEST_F(AdaptorICSTest, resource_type_test_deleter_deletes_correct_handle)
{
    using namespace testing;

    native_handle_t test_native_handle;
    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, _, _, _, _, _, _))
    .WillOnce(DoAll(
                  SetArgPointee<5>((native_handle_t*)&test_native_handle),
                  SetArgPointee<6>(width.as_uint32_t()*4),
                  Return(0)));
    EXPECT_CALL(*mock_alloc_device, free_interface( _, &test_native_handle) );

    alloc_adaptor->alloc_buffer(width, height, pf, usage );
}

TEST_F(AdaptorICSTest, adaptor_gralloc_format_conversion_rgba8888)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, _, _, HAL_PIXEL_FORMAT_RGBA_8888, _, _, _));
    EXPECT_CALL(*mock_alloc_device, free_interface( _, _) );

    alloc_adaptor->alloc_buffer(width, height, pf, usage );
}

TEST_F(AdaptorICSTest, adaptor_gralloc_dimension_conversion)
{
    using namespace testing;
    int w = (int) width.as_uint32_t();
    int h = (int) height.as_uint32_t();
    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, w, h, _, _, _, _));
    EXPECT_CALL(*mock_alloc_device, free_interface( _, _) );

    alloc_adaptor->alloc_buffer(width, height, pf, usage );
}

TEST_F(AdaptorICSTest, adaptor_gralloc_usage_conversion)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface( _, _, _, _,
                (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER), _, _));
    EXPECT_CALL(*mock_alloc_device, free_interface( _, _) );
    
    alloc_adaptor->alloc_buffer(width, height, pf, usage );
}
