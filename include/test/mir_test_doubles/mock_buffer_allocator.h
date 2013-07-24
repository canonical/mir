/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_TEST_DOUBLES_MOCK_BUFFER_ALLOCATOR_H_
#define MIR_TEST_DOUBLES_MOCK_BUFFER_ALLOCATOR_H_

#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockBufferAllocator : public compositor::GraphicBufferAllocator
{
    MockBufferAllocator()
    {
        using namespace testing;
        ON_CALL(*this, alloc_buffer(_))
            .WillByDefault(Return(std::make_shared<StubBuffer>()));
    }
    ~MockBufferAllocator() noexcept{}

    MOCK_METHOD1(alloc_buffer, std::shared_ptr<graphics::Buffer>(compositor::BufferProperties const&));
    MOCK_METHOD0(supported_pixel_formats, std::vector<geometry::PixelFormat>());
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_BUFFER_ALLOCATOR_H_
