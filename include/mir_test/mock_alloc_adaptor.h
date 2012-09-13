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

#include "mir/graphics/android/graphic_alloc_adaptor.h"
#include "mir/graphics/android/android_buffer.h"

#include <system/window.h>
#include <gmock/gmock.h>

namespace mir
{
namespace graphics
{
namespace android
{

static native_handle_t* mock_generate_sane_android_handle()
{
    native_handle_t *handle;
    int numFd=2;
    int numInt = 9;
    int total = numFd + numInt;
    int header_offset = 3; 
    handle = (native_handle_t*) malloc(sizeof(int) * (header_offset+ total));
    handle->version = 0x389;
    handle->numFds = numFd;
    handle->numInts = numInt;

    for(int i=0; i<total; i++)
    {
        handle->data[i] = i*3;
    }

    return handle;
}

class MockBufferHandle : public AndroidBufferHandle
{
public:
    MockBufferHandle(native_handle_t* native_handle)
     : handle(native_handle)
    {
        using namespace testing;
        ON_CALL(*this, get_egl_client_buffer())
            .WillByDefault(Return((EGLClientBuffer)handle));
    }

    MOCK_CONST_METHOD0(get_egl_client_buffer, EGLClientBuffer());
    MOCK_CONST_METHOD0(height, geometry::Height());
    MOCK_CONST_METHOD0(width,  geometry::Width());
    MOCK_CONST_METHOD0(stride, geometry::Stride());
    MOCK_CONST_METHOD0(format, compositor::PixelFormat());
    MOCK_CONST_METHOD0(usage,  BufferUsage());
    MOCK_CONST_METHOD0(get_ipc_package,  std::shared_ptr<compositor::BufferIPCPackage>());
    
    native_handle_t *handle;
};

class MockAllocAdaptor : public GraphicAllocAdaptor
{
public:
    MockAllocAdaptor(std::shared_ptr<MockBufferHandle> mock)
        :
        mock_handle(mock)
    {
        using namespace testing;

        ON_CALL(*this, alloc_buffer(_,_,_,_))
        .WillByDefault(Return(mock_handle));
    }

    MOCK_METHOD4(alloc_buffer, std::shared_ptr<AndroidBufferHandle>(geometry::Width, geometry::Height, compositor::PixelFormat, BufferUsage));
    MOCK_METHOD2(inspect_buffer, bool(char*, int));

    std::shared_ptr<AndroidBufferHandle> mock_handle;
};

}
}
}
