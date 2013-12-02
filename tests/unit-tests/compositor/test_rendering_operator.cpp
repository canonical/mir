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

#include "src/server/compositor/rendering_operator.h"
#include "mir/geometry/rectangle.h"
#include "mir_test_doubles/mock_surface_renderer.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_compositing_criteria.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;

TEST(RenderingOperator, render_operator_saves_resources)
{
    using namespace testing;

    unsigned long frameno = 84;
    mtd::MockSurfaceRenderer mock_renderer;
    mtd::MockCompositingCriteria mock_criteria;
    mtd::MockBufferStream mock_stream;
    auto stub_buffer0 = std::make_shared<mtd::StubBuffer>();
    auto stub_buffer1 = std::make_shared<mtd::StubBuffer>();
    auto stub_buffer2 = std::make_shared<mtd::StubBuffer>();

    EXPECT_CALL(mock_stream, lock_compositor_buffer(frameno))
        .Times(3)
        .WillOnce(Return(stub_buffer0))
        .WillOnce(Return(stub_buffer1)) 
        .WillOnce(Return(stub_buffer2));

    Sequence seq;
    EXPECT_CALL(mock_renderer, render(Ref(mock_criteria), Ref(*stub_buffer0)))
        .InSequence(seq);
    EXPECT_CALL(mock_renderer, render(Ref(mock_criteria), Ref(*stub_buffer1)))
        .InSequence(seq);
    EXPECT_CALL(mock_renderer, render(Ref(mock_criteria), Ref(*stub_buffer2)))
        .InSequence(seq);
 
    std::vector<std::shared_ptr<void>> saved_resources;

    {
        auto save_fn = [&](std::shared_ptr<void> const& r) { saved_resources.push_back(r); };
        mc::RenderingOperator rendering_operator(mock_renderer, save_fn, frameno);

        rendering_operator(mock_criteria, mock_stream);
        rendering_operator(mock_criteria, mock_stream);
        rendering_operator(mock_criteria, mock_stream);
    }

    ASSERT_EQ(3u, saved_resources.size());
    EXPECT_EQ(stub_buffer0, saved_resources[0]);
    EXPECT_EQ(stub_buffer1, saved_resources[1]);
    EXPECT_EQ(stub_buffer2, saved_resources[2]);
}
