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

#include "mir_platform/android/android_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace geom=mir::geometry;


class AllocDevice 
{
    public:
    virtual int  mock_alloc(alloc_device_t*, int, int, int, int, buffer_handle_t*, int*) = 0;
    virtual int  mock_free(alloc_device_t*, buffer_handle_t handle) = 0;
    virtual void mock_dump(alloc_device_t*,  char *buf, int buf_len) = 0;
};

struct MockAllocDevice : public AllocDevice,
                         public alloc_device_t 
{
    public:
    MockAllocDevice()
    {
          alloc = hook_alloc;
          free = hook_free;
          dump = hook_dump; 
    }

    static int hook_alloc(alloc_device_t* mock_alloc,
                           int w, int h, int format, int usage,
                           buffer_handle_t* handle, int* stride)
    {
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        return mocker->mock_alloc(mock_alloc, w, h, format, usage, handle, stride);
    }

    static int hook_free(alloc_device_t* mock_alloc, buffer_handle_t handle)
    {
        printf("HOOKFREE\n");
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        return mocker->mock_free(mock_alloc, handle);
    }

    static void hook_dump(alloc_device_t* mock_alloc, char* buf, int buf_len)
    {
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        return mocker->mock_dump(mock_alloc, buf, buf_len);
    }
 
    MOCK_METHOD7(mock_alloc, int(alloc_device_t*, int, int, int, int, buffer_handle_t*, int*) );
    MOCK_METHOD2(mock_free,  int(alloc_device_t*, buffer_handle_t)); 
    MOCK_METHOD3(mock_dump, void(alloc_device_t*, char*, int));

};

TEST(AndroidGraphicsBuffer, struct_mock) 
{
    MockAllocDevice mock_alloc;

    EXPECT_CALL(mock_alloc, mock_free(&mock_alloc, NULL));
    mock_alloc.free(&mock_alloc,NULL);
}


TEST(AndroidGraphicsBuffer, creation)
{
    using namespace testing;

    MockAllocDevice mock_alloc_device;
    struct alloc_device_t* mock_alloc_device_struct = &mock_alloc_device;

    unsigned int w = 300, h = 200;
    buffer_handle_t dummy_handle;

    /* todo: get rid of magic numbers, figure out which android headers have these values */
    EXPECT_CALL(mock_alloc_device, mock_alloc(&mock_alloc_device, w, h,
                                             4, 0x300, _, _ ))
                .WillOnce(DoAll(
                            SetArgPointee<5>(dummy_handle),
                            SetArgPointee<6>(w*4),
                            Return(0)));

    EXPECT_CALL(mock_alloc_device, mock_free(&mock_alloc_device, dummy_handle));

    mc::Buffer* buffer = new mg::AndroidBuffer(mock_alloc_device_struct, geom::Width(w), geom::Height(h), mc::PixelFormat::rgba_8888);

    EXPECT_NE((int)buffer, NULL);

    delete buffer;
}

















