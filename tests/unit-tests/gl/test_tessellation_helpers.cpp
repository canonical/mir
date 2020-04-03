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

struct BoundingBox
{
    float left, right, top, bottom;

    static auto from(geom::Rectangle const& rect) -> BoundingBox
    {
        return BoundingBox{
            static_cast<float>(rect.left().as_int()),
            static_cast<float>(rect.right().as_int()),
            static_cast<float>(rect.top().as_int()),
            static_cast<float>(rect.bottom().as_int())};
    }

    auto operator==(BoundingBox const& other) const -> bool
    {
        return left == other.left &&
               right == other.right &&
               top == other.top &&
               bottom == other.bottom;
    }
};

auto operator<<(std::ostream& os, const BoundingBox& bb) -> std::ostream&
{
    os << "{ left=" << bb.left << ", right=" << bb.right << ", top=" << bb.top << ", bottom=" << bb.bottom << " }";
    return os;
}

class Tessellation : public testing::Test
{
public:
    void SetUp() override
    {
        ON_CALL(renderable, screen_position())
            .WillByDefault(Return(rect));
    }

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
                EXPECT_THAT(tex_coord, AnyOf(Eq(1.0f), Eq(0.0f))) << "axis=" << axis;
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
    EXPECT_THAT(box, Eq(BoundingBox::from(rect)));
}

TEST_F(Tessellation, bounding_box_gets_offset)
{
    geom::Displacement const offset{3, 8};
    mgl::Primitive const primitive = mgl::tessellate_renderable_into_rectangle(renderable, offset);
    auto const box = bounding_box(primitive);
    geom::Rectangle const expected{rect.top_left - offset, rect.size};
    EXPECT_THAT(box, Eq(BoundingBox::from(expected)));
}

TEST_F(Tessellation, vertex_zs_are_1)
{
    int const x{3}, y{8};
    mgl::Primitive const primitive = mgl::tessellate_renderable_into_rectangle(renderable, {x, y});
    for (int i = 0; i < primitive.nvertices; i++)
    {
        EXPECT_THAT(primitive.vertices[i].position[2], Eq(0)) << "for i = " << i;
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
