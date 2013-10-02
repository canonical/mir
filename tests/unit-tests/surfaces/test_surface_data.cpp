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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/surfaces/surface_data.h"

#include "mir/geometry/rectangle.h"

#include "mir_test_doubles/mock_surface_state.h"
#include "mir_test/fake_shared.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mi = mir::input;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;

namespace
{
class MockCallback
{
public:
    MOCK_METHOD0(call, void());
};

struct SurfaceDataTest : public testing::Test
{
    void SetUp()
    {
        name = std::string("aa");
        top_left = geom::Point{geom::X{4}, geom::Y{7}};
        size = geom::Size{5, 9};
        rect = geom::Rectangle{top_left, size};
        null_change_cb = []{};
        mock_change_cb = std::bind(&MockCallback::call, &mock_callback);
    }
    std::string name;
    geom::Point top_left;
    geom::Size size;
    geom::Rectangle rect;

    MockCallback mock_callback;
    std::function<void()> null_change_cb;
    std::function<void()> mock_change_cb;
};

}

TEST_F(SurfaceDataTest, basics)
{ 
    ms::SurfaceData data{name, rect, null_change_cb};
    EXPECT_EQ(name, data.name());
    EXPECT_EQ(rect.size, data.size());
    EXPECT_EQ(rect.top_left, data.position());
}

TEST_F(SurfaceDataTest, update_position)
{ 
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::SurfaceData storage{name, rect, mock_change_cb};
    EXPECT_EQ(rect.top_left, storage.position());

    auto new_top_left = geom::Point{geom::X{6}, geom::Y{10}};
    storage.move_to(new_top_left);
    EXPECT_EQ(new_top_left, storage.position());
}

TEST_F(SurfaceDataTest, test_surface_set_rotation_updates_transform)
{
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::SurfaceData storage{name, rect, mock_change_cb};
    auto original_transformation = storage.transformation();

    storage.apply_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});
    auto rotated_transformation = storage.transformation();
    EXPECT_NE(original_transformation, rotated_transformation);
}

TEST_F(SurfaceDataTest, test_surface_transformation_cache_refreshes)
{
    using namespace testing;

    const geom::Size sz{geom::Width{85}, geom::Height{43}};
    const geom::Rectangle origin{geom::Point{geom::X{77}, geom::Y{88}}, sz};
    const geom::Rectangle moved_pt{geom::Point{geom::X{55}, geom::Y{66}}, sz};
    ms::SurfaceData storage{name, origin, null_change_cb};

    glm::mat4 t0 = storage.transformation();
    storage.move_to(moved_pt.top_left);
    EXPECT_NE(t0, storage.transformation());
    storage.move_to(origin.top_left);
    EXPECT_EQ(t0, storage.transformation());

    storage.apply_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 t1 = storage.transformation();
    EXPECT_NE(t0, t1);
}

TEST_F(SurfaceDataTest, test_surface_set_alpha_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::SurfaceData surface_state{name, rect, mock_change_cb};

    float alpha = 0.5f;
    surface_state.apply_alpha(0.5f);
    EXPECT_THAT(alpha, FloatEq(surface_state.alpha()));
}

TEST_F(SurfaceDataTest, test_surface_is_opaque_by_default)
{
    using namespace testing;
    ms::SurfaceData surface_state{name, rect, null_change_cb};
    EXPECT_THAT(1.0f, FloatEq(surface_state.alpha()));
}

TEST_F(SurfaceDataTest, test_surface_apply_rotation)
{
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::SurfaceData surface_state{name, rect, mock_change_cb};
    surface_state.apply_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});
}

TEST_F(SurfaceDataTest, test_surface_should_be_rendered_in)
{
    ms::SurfaceData surface_state{name, rect, mock_change_cb};
    geom::Rectangle output_rect{geom::Point{0,0}, geom::Size{100, 100}};

    //not renderable by default
    EXPECT_FALSE(surface_state.should_be_rendered_in(rect));

    surface_state.set_hidden(false);
    //not renderable if no first frame has been posted by client, regardless of hide state
    EXPECT_FALSE(surface_state.should_be_rendered_in(output_rect));
    surface_state.set_hidden(true);
    EXPECT_FALSE(surface_state.should_be_rendered_in(output_rect));

    surface_state.frame_posted();
    EXPECT_FALSE(surface_state.should_be_rendered_in(output_rect));

    surface_state.set_hidden(false);
    EXPECT_TRUE(surface_state.should_be_rendered_in(output_rect));

    // Not renderable if not overlapping with supplied rect
    geom::Rectangle output_rect1{geom::Point{100,100}, geom::Size{100, 100}};
    EXPECT_FALSE(surface_state.should_be_rendered_in(output_rect1));
}

TEST_F(SurfaceDataTest, test_surface_hidden_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::SurfaceData surface_state{name, rect, mock_change_cb};
    surface_state.set_hidden(true);
}

TEST_F(SurfaceDataTest, test_surface_frame_posted_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::SurfaceData surface_state{name, rect, mock_change_cb};
    surface_state.frame_posted();
}

// a 1x1 window at (1,1) will get events at (1,1)
TEST_F(SurfaceDataTest, default_region_is_surface_rectangle)
{
    geom::Point pt(1,1);
    geom::Size one_by_one{geom::Width{1}, geom::Height{1}};
    ms::SurfaceData surface_state{name, geom::Rectangle{pt, one_by_one}, mock_change_cb};

    std::vector<geom::Point> contained_pt
    {
        geom::Point{geom::X{1}, geom::Y{1}}
    };

    for(auto x = 0; x <= 3; x++)
    {
        for(auto y = 0; y <= 3; y++)
        {
            auto test_pt = geom::Point{x, y};
            auto contains = surface_state.contains(test_pt);
            if (std::find(contained_pt.begin(), contained_pt.end(), test_pt) != contained_pt.end())
            {
                EXPECT_TRUE(contains);
            }
            else
            {
                EXPECT_FALSE(contains);
            }
        }
    }
}

TEST_F(SurfaceDataTest, set_input_region)
{
    std::vector<geom::Rectangle> const rectangles = {
        {{geom::X{0}, geom::Y{0}}, {geom::Width{1}, geom::Height{1}}}, //region0
        {{geom::X{1}, geom::Y{1}}, {geom::Width{1}, geom::Height{1}}}  //region1
    };

    ms::SurfaceData surface_state{name, rect, mock_change_cb};
    surface_state.set_input_region(rectangles);

    std::vector<geom::Point> contained_pt
    {
        //region0 points
        geom::Point{geom::X{0}, geom::Y{0}},
        //region1 points
        geom::Point{geom::X{1}, geom::Y{1}},
    };

    for(auto x = 0; x <= 3; x++)
    {
        for(auto y = 0; y <= 3; y++)
        {
            auto test_pt = geom::Point{x, y};
            auto contains = surface_state.contains(test_pt);
            if (std::find(contained_pt.begin(), contained_pt.end(), test_pt) != contained_pt.end())
            {
                EXPECT_TRUE(contains);
            }
            else
            {
                EXPECT_FALSE(contains);
            }
        }
    }
}
