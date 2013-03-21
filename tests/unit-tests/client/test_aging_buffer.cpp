/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/client/aging_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace geom = mir::geometry;

namespace mir_toolkit
{
    struct MirBufferPackage;
}

namespace mir
{
namespace test
{

struct MockAgingBuffer : public mcl::AgingBuffer
{
    MockAgingBuffer()
    {
    }

    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, geom::PixelFormat());
    MOCK_CONST_METHOD0(get_buffer_package, std::shared_ptr<mir_toolkit::MirBufferPackage>());
    MOCK_METHOD0(get_native_handle, MirNativeBuffer());
};

TEST(MirClientAgingBufferTest, buffer_age_starts_at_zero)
{
    using namespace testing;

    auto mock_buffer = std::make_shared<NiceMock<MockAgingBuffer>>();

    EXPECT_EQ(0u, mock_buffer->age());
}

TEST(MirClientAgingBufferTest, buffer_age_set_to_one_on_submit)
{
    using namespace testing;

    auto mock_buffer = std::make_shared<NiceMock<MockAgingBuffer>>();
    mock_buffer->mark_as_submitted();

    EXPECT_EQ(1u, mock_buffer->age());    
}

TEST(MirClientAgingBufferTest, buffer_age_increases_on_increment)
{
    using namespace testing;

    auto mock_buffer = std::make_shared<NiceMock<MockAgingBuffer>>();
    mock_buffer->mark_as_submitted();

    for (uint32_t age = 2; age < 10; ++age)
    {
        mock_buffer->increment_age();
        EXPECT_EQ(age, mock_buffer->age());
    }
}

TEST(MirClientAgingBufferTest, incrementing_age_has_no_effect_for_unsubmitted_buffer)
{
    using namespace testing;

    auto mock_buffer = std::make_shared<NiceMock<MockAgingBuffer>>();
    mock_buffer->increment_age();

    EXPECT_EQ(0u, mock_buffer->age());
}

}
}
