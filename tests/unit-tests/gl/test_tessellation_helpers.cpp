/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include <glm/gtc/matrix_transform.hpp>
#include <mir/gl/tessellation_helpers.h>
#include <mir/test/doubles/mock_renderable.h>

using namespace testing;

namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mgl = mir::gl;

class Tessellation : public testing::Test
{
public:
    void SetUp() override
    {
        ON_CALL(renderable, screen_position())
            .WillByDefault(Return(rect));
    }

    struct BoundingBox
    {
        float left, right, top, bottom;
    };

    static auto bounding_box(mgl::Primitive const& primitive) -> BoundingBox
    {
        BoundingBox box{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::min()};

        for (int i = 0; i < primitive.nvertices; i++)
        {
            box.left    = std::min(box.left,    primitive.vertices[i].position[0]);
            box.right   = std::max(box.right,   primitive.vertices[i].position[0]);
            box.top     = std::min(box.top,     primitive.vertices[i].position[1]);
            box.bottom  = std::max(box.bottom,  primitive.vertices[i].position[1]);
        }

        return box;
    }

    static void expect_tex_coords_1_or_0(mgl::Primitive const& primitive)
    {
        for (int i = 0; i < primitive.nvertices; i++)
        {
            for (int axis = 0; axis < 2; axis++)
            {
                float const tex_coord = primitive.vertices[i].texcoord[axis];
                EXPECT_THAT(tex_coord == 0.0 || tex_coord == 1.0, Eq(true));
            }
        }
    }

    geom::Rectangle const rect{{4, 6}, {10, 20}};
    NiceMock<mtd::MockRenderable> const renderable;
};

TEST_F(Tessellation, has_4_vertices)
{
    mgl::Primitive const primitive = mgl::tessellate_renderable_into_rectangle(renderable, {});
    EXPECT_THAT(primitive.type, Eq(GL_TRIANGLE_STRIP));
    EXPECT_THAT(primitive.nvertices, Eq(4));
}

TEST_F(Tessellation, has_correct_bounding_box)
{
    mgl::Primitive const primitive = mgl::tessellate_renderable_into_rectangle(renderable, {});
    auto const box = bounding_box(primitive);
    EXPECT_THAT(box.left, Eq(rect.top_left.x.as_int()));
    EXPECT_THAT(box.top, Eq(rect.top_left.y.as_int()));
    EXPECT_THAT(box.right, Eq(rect.right().as_int()));
    EXPECT_THAT(box.bottom, Eq(rect.bottom().as_int()));
}

TEST_F(Tessellation, bounding_box_gets_offset)
{
    int const x{3}, y{8};
    mgl::Primitive const primitive = mgl::tessellate_renderable_into_rectangle(renderable, {x, y});
    auto const box = bounding_box(primitive);
    EXPECT_THAT(box.left, Eq(rect.top_left.x.as_int() - x));
    EXPECT_THAT(box.top, Eq(rect.top_left.y.as_int() - y));
    EXPECT_THAT(box.right, Eq(rect.right().as_int() - x));
    EXPECT_THAT(box.bottom, Eq(rect.bottom().as_int() - y));
}

TEST_F(Tessellation, vertex_zs_are_1)
{
    int const x{3}, y{8};
    mgl::Primitive const primitive = mgl::tessellate_renderable_into_rectangle(renderable, {x, y});
    for (int i = 0; i < primitive.nvertices; i++)
    {
        EXPECT_THAT(primitive.vertices[i].position[2], Eq(0));
    }
}

TEST_F(Tessellation, tex_coords_0_or_1)
{
    int const x{3}, y{8};
    mgl::Primitive const primitive = mgl::tessellate_renderable_into_rectangle(renderable, {x, y});
    expect_tex_coords_1_or_0(primitive);
}

TEST_F(Tessellation, tex_coords_0_or_1_when_transformed)
{
    int const x{3}, y{8};
    ON_CALL(renderable, transformation())
        .WillByDefault(testing::Return(glm::scale(glm::mat4(1.0f), glm::vec3(2.5f))));
    mgl::Primitive const primitive = mgl::tessellate_renderable_into_rectangle(renderable, {x, y});
    expect_tex_coords_1_or_0(primitive);
}
