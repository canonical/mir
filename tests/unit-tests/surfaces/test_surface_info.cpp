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

#include "mir/surfaces/surface_data_storage.h"

#include <gtest/gtest.h>

namespace ms = mir::surfaces;
namespace geom = mir::geometry;

namespace
{
class MockCallback
{
public:
    MOCK_METHOD0(call, void());
};
}

struct SurfaceInfoTest : public testing::Test
{
    void SetUp()
    {
        name = std::string("aa");
        top_left = geom::Point{geom::X{4}, geom::Y{7}};
        size = geom::Size{geom::Width{5}, geom::Height{9}};
        null_change_cb = []{};
        mock_change_cb = std::bind(&MockCallback::call, &mock_callback);
    }
    std::string name;
    geom::Point top_left;
    geom::Size size;

    MockCallback mock_callback;
    std::function<void()> null_change_cb;
    std::function<void()> mock_change_cb;
};

TEST_F(SurfaceInfoTest, basics)
{ 
    geom::Rectangle rect{top_left, size};
    ms::SurfaceDataStorage storage{name, top_left, size, null_change_cb};
    EXPECT_EQ(name, storage.name());
    EXPECT_EQ(rect, storage.size_and_position());
}

TEST_F(SurfaceInfoTest, update_position)
{ 
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    geom::Rectangle rect1{top_left, size};
    ms::SurfaceDataStorage storage{name, top_left, size, mock_change_cb};
    EXPECT_EQ(rect1, storage.size_and_position());

    auto new_top_left = geom::Point{geom::X{5}, geom::Y{10}};
    geom::Rectangle rect2{new_top_left, size};
    storage.set_top_left(new_top_left);
    EXPECT_EQ(rect2, storage.size_and_position());
}

TEST_F(SurfaceInfoTest, test_surface_set_rotation_updates_transform)
{
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    geom::Size s{geom::Width{55}, geom::Height{66}};
    ms::SurfaceDataStorage storage{name, top_left, s, mock_change_cb};
    auto original_transformation = storage.transformation();

    storage.apply_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});
    auto rotated_transformation = storage.transformation();
    EXPECT_NE(original_transformation, rotated_transformation);
}

TEST_F(SurfaceInfoTest, test_surface_transformation_cache_refreshes)
{
    using namespace testing;

    const geom::Size sz{geom::Width{85}, geom::Height{43}};
    const geom::Rectangle origin{geom::Point{geom::X{77}, geom::Y{88}}, sz};
    const geom::Rectangle moved_pt{geom::Point{geom::X{55}, geom::Y{66}}, sz};
    ms::SurfaceDataStorage storage{name, origin.top_left, sz};

    glm::mat4 t0 = storage.transformation();
    storage.set_top_left(moved_pt.top_left);
    EXPECT_NE(t0, storage.transformation());
    storage.set_top_left(origin.top_left);
    EXPECT_EQ(t0, storage.transformation());

    storage.apply_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 t1 = storage.transformation();
    EXPECT_NE(t0, t1);
}
