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

#include <gmock/gmock.h>

namespace mir
{
namespace graphics
{
namespace android
{

struct AndroidBufferHandleEmptyDeleter
{
    void operator()(BufferHandle*)
    {
    }
};

class MockBufferHandle : public BufferHandle
{
public:
    MOCK_METHOD0(get_egl_client_buffer, EGLClientBuffer());
    MOCK_METHOD0(height, geometry::Height());
    MOCK_METHOD0(width,  geometry::Width()); 
    MOCK_METHOD0(stride, geometry::Stride());
    MOCK_METHOD0(format, compositor::PixelFormat());
    MOCK_METHOD0(usage,  BufferUsage());
};

class MockAllocAdaptor : public GraphicAllocAdaptor
{
public:
    MockAllocAdaptor()
    {
        using namespace testing;

        mock_handle = std::make_shared<MockBufferHandle>();
        ON_CALL(*this, alloc_buffer(_,_,_,_))
        .WillByDefault(Return(mock_handle));
    }

    MOCK_METHOD4(alloc_buffer, std::shared_ptr<BufferHandle>(geometry::Width, geometry::Height, compositor::PixelFormat, BufferUsage));
    MOCK_METHOD2(inspect_buffer, bool(char*, int));

    std::shared_ptr<BufferHandle> mock_handle;
};

}
}
}
