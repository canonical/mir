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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_COMPOSITOR_MOCK_GRAPHIC_BUFFER_ALLOCATOR_H_
#define MIR_COMPOSITOR_MOCK_GRAPHIC_BUFFER_ALLOCATOR_H_

#include "mir/compositor/graphic_buffer_allocator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mir
{
namespace compositor
{

void test() {}

struct MockGraphicBufferAllocator : mc::GraphicBufferAllocator
{
 public:
    MockGraphicBufferAllocator() 
    {
        using namespace testing;

        std::unique_ptr<Buffer> testbuffer(nullptr);

        ON_CALL(*this, alloc_buffer(_,_,_))
                .WillByDefault(ReturnRef(testbuffer));

    }

    MOCK_METHOD3(alloc_buffer, std::unique_ptr<Buffer>& (geometry::Width, geometry::Height, PixelFormat));
    MOCK_METHOD1(free_buffer, void(std::shared_ptr<Buffer>));
};

}
}

#endif // MIR_COMPOSITOR_MOCK_GRAPHIC_BUFFER_ALLOCATOR_H_
