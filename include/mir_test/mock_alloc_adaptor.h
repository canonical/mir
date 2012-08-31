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

class MockAllocAdaptor : public GraphicAllocAdaptor
{
public:
    MockAllocAdaptor()
    {
        using namespace testing;

        ON_CALL(*this, alloc_buffer(_,_,_,_,_,_))
        .WillByDefault(Return(true));
    }

    MOCK_METHOD6(alloc_buffer, bool(std::shared_ptr<BufferHandle>&, geometry::Stride&, geometry::Width, geometry::Height, compositor::PixelFormat, BufferUsage));
    MOCK_METHOD2(inspect_buffer, bool(char*, int));

};

}
}
