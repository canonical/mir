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

#include "mir/graphics/graphic_alloc_adaptor.h"
#include "mir/graphics/android/android_buffer.h"

#include <gmock/gmock.h>

namespace mir
{
namespace graphics
{

struct BufferHandleEmptyDeleter
{
    void operator()(BufferHandle*)
    {
    }
};
class MockAllocAdaptor : public GraphicAllocAdaptor
{
public:
    MockAllocAdaptor()
    {
        using namespace testing;

        fake_handle = (BufferHandle*) 0x498a;
        BufferHandleEmptyDeleter del;
        ON_CALL(*this, alloc_buffer(_,_,_,_))
        .WillByDefault(Return(std::shared_ptr<BufferHandle>(fake_handle, del)));
    }

    MOCK_METHOD4(alloc_buffer, std::shared_ptr<BufferHandle>(geometry::Width, geometry::Height, compositor::PixelFormat, BufferUsage));
    MOCK_METHOD2(inspect_buffer, bool(char*, int));

    BufferHandle* fake_handle;
};

}
}
