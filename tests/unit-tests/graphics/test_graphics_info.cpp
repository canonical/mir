/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir_test_doubles/mock_surface_info.h"
#include "mir_test/fake_shared.h"

#include "mir/graphics/surface_state.h"
    
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace geom = mir::geometry;

namespace
{

class MockCallback
{
public:
    MOCK_METHOD0(call, void());
};


struct SurfaceGraphicsState : public ::testing::Test
{
    void SetUp()
    {
        using namespace testing;
        null_change_cb = []{};
        mock_change_cb = std::bind(&MockCallback::call, &mock_callback);

        static glm::mat4 the_matrix;
        ON_CALL(primitive_info, transformation())
            .WillByDefault(ReturnRef(the_matrix)); 
        ON_CALL(primitive_info, size_and_position())
            .WillByDefault(Return(primitive_rect));
    }

    geom::Point primitive_pt{geom::X{1}, geom::Y{1}};
    geom::Size primitive_sz{geom::Width{1}, geom::Height{1}};
    geom::Rectangle primitive_rect{primitive_pt, primitive_sz};
    mtd::MockSurfaceInfo primitive_info;

    MockCallback mock_callback;
    std::function<void()> null_change_cb;
    std::function<void()> mock_change_cb;
};
}

TEST_F(SurfaceGraphicsState, test_surface_set_alpha_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    mg::SurfaceState surface_state(mt::fake_shared(primitive_info), mock_change_cb);

    float alpha = 0.5f;
    surface_state.apply_alpha(0.5f);
    EXPECT_THAT(alpha, FloatEq(surface_state.alpha()));
}

TEST_F(SurfaceGraphicsState, test_surface_is_opaque_by_default)
{
    using namespace testing;
    mg::SurfaceState surface_state(mt::fake_shared(primitive_info), mock_change_cb);
    EXPECT_THAT(1.0f, FloatEq(surface_state.alpha()));
}

TEST_F(SurfaceGraphicsState, test_surface_get_transformation)
{
    using namespace testing;
    EXPECT_CALL(primitive_info, transformation())
        .Times(1);

    mg::SurfaceState surface_state(mt::fake_shared(primitive_info), mock_change_cb);
    surface_state.transformation();
}

TEST_F(SurfaceGraphicsState, test_surface_apply_rotation)
{
    using namespace testing;
    InSequence seq;
    EXPECT_CALL(primitive_info, apply_rotation(FloatEq(60.0f),glm::vec3{0.0f, 0.0f, 1.0f}))
        .Times(1);
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    mg::SurfaceState surface_state(mt::fake_shared(primitive_info), mock_change_cb);
    surface_state.apply_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});
}

#if 0
TEST_F(SurfaceGraphicsState, test_surface_set_rotation_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);
    mg::SurfaceState surface_state(mt::fake_shared(primitive_info), mock_change_callback);

    surface_state.apply_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});
}

TEST_F(SurfaceGraphicsState, test_surface_set_rotation)
{
    using namespace testing;

    ms::Surface surf(mock_basic_info, mock_input_info, mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(), null_change_cb);
    surf.set_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});

    geom::Size s{geom::Width{55}, geom::Height{66}};
    ON_CALL(*mock_basic_info, size_and_position())
        .WillByDefault(Return(geom::Rectangle{geom::Point{geom::X{}, geom::Y{}},s}));

    auto ret_transformation = surf.transformation();

    EXPECT_NE(glm::mat4(), ret_transformation);
}

TEST_F(SurfaceGraphicsState, test_surface_transformation_cache_refreshes)
{
    using namespace testing;

    const geom::Size sz{geom::Width{85}, geom::Height{43}};
    const geom::Rectangle origin{geom::Point{geom::X{77}, geom::Y{88}}, sz};
    const geom::Rectangle moved_pt{geom::Point{geom::X{55}, geom::Y{66}}, sz};

    Sequence seq;
    EXPECT_CALL(*mock_basic_info, size_and_position())
        .InSequence(seq)
        .WillOnce(Return(origin));
    EXPECT_CALL(*mock_basic_info, size_and_position())
        .InSequence(seq)
        .WillOnce(Return(moved_pt));
    EXPECT_CALL(*mock_basic_info, size_and_position())
        .InSequence(seq)
        .WillOnce(Return(origin));
    EXPECT_CALL(*mock_basic_info, size_and_position())
        .InSequence(seq)
        .WillOnce(Return(origin));

    ms::Surface surf(mock_basic_info, mock_input_info, mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(), null_change_cb);
    glm::mat4 t0 = surf.transformation();
    surf.move_to(moved_pt.top_left);
    EXPECT_NE(t0, surf.transformation());
    surf.move_to(origin.top_left);
    EXPECT_EQ(t0, surf.transformation());

    surf.set_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 t1 = surf.transformation();
    EXPECT_NE(t0, t1);
}

TEST_F(SurfaceGraphicsState, test_surface_set_alpha)
{
    using namespace testing;

    ms::Surface surf(mock_basic_info, mock_input_info, mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(), null_change_cb);
    float alpha = 0.67f;

    surf.set_alpha(alpha);
    auto ret_alpha = surf.alpha();

    EXPECT_EQ(alpha, ret_alpha);
}

TEST_F(SurfaceGraphicsState, test_surface_next_buffer_does_not_set_valid_until_second_frame)
{
    ms::Surface surf(mock_basic_info, mock_input_info, mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(), mock_change_cb);

    EXPECT_FALSE(surf.should_be_rendered());
    surf.advance_client_buffer();
    EXPECT_FALSE(surf.should_be_rendered());
    surf.advance_client_buffer();
    EXPECT_TRUE(surf.should_be_rendered());
}

TEST_F(SurfaceGraphicsState, flag_for_render_makes_surfaces_valid)
{
    using namespace testing;

    ms::Surface surf(mock_basic_info, mock_input_info, mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(), mock_change_cb);

    EXPECT_FALSE(surf.should_be_rendered());

    surf.flag_for_render();

    EXPECT_TRUE(surf.should_be_rendered());
}
#endif
