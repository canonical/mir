/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/frontend_wayland/input_method_v1.h"

#include <mir/test/doubles/stub_surface.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mw = mir::wayland;
namespace mtd = mir::test::doubles;
using namespace testing;

namespace
{

class FakeInputPanelSurface : public mtd::StubSurface
{
public:
    auto content_size() const -> geom::Size override
    {
        return size;
    }

    auto get_input_region() const -> std::vector<geom::Rectangle> override
    {
        return input_region;
    }

    void set_input_region(std::vector<geom::Rectangle> const& region) override
    {
        input_region = region;
    }

    void resize(geom::Size const& new_size) override
    {
        size = new_size;
    }

private:
    geom::Size size{800, 400};
    std::vector<geom::Rectangle> input_region;
};

class InputPanelV1Test : public Test
{
public:
    FakeInputPanelSurface surface;
};

}

TEST_F(InputPanelV1Test, empty_region_sentinel_produces_no_exclusive_rect)
{
    surface.set_input_region({{{0, 0}, {0, 0}}});

    EXPECT_THAT(mf::input_panel_exclusive_rect(surface), Eq(std::nullopt));
}

TEST_F(InputPanelV1Test, degenerate_zero_area_rects_produce_no_exclusive_rect)
{
    surface.set_input_region({
        {{10, 200}, {0, 100}},
        {{10, 200}, {300, 0}},
    });

    EXPECT_THAT(mf::input_panel_exclusive_rect(surface), Eq(std::nullopt));
}

TEST_F(InputPanelV1Test, single_non_empty_input_rect_is_used_as_exclusive_rect)
{
    geom::Rectangle const keyboard_rect{{0, 200}, {800, 200}};
    surface.set_input_region({keyboard_rect});

    EXPECT_THAT(mf::input_panel_exclusive_rect(surface), Optional(Eq(keyboard_rect)));
}

TEST_F(InputPanelV1Test, multiple_input_rects_use_bounding_rectangle)
{
    surface.set_input_region({
        {{0, 300}, {800, 100}},
        {{100, 200}, {600, 100}},
    });

    EXPECT_THAT(
        mf::input_panel_exclusive_rect(surface),
        Optional(Eq(geom::Rectangle{{0, 200}, {800, 200}})));
}

TEST_F(InputPanelV1Test, empty_rects_are_ignored_when_computing_bounding_rectangle)
{
    surface.set_input_region({
        {{0, 0}, {0, 0}},
        {{50, 250}, {700, 150}},
    });

    EXPECT_THAT(
        mf::input_panel_exclusive_rect(surface),
        Optional(Eq(geom::Rectangle{{50, 250}, {700, 150}})));
}

TEST_F(InputPanelV1Test, empty_input_region_list_falls_back_to_surface_local_content_size)
{
    surface.set_input_region({});
    surface.resize({800, 320});

    EXPECT_THAT(
        mf::input_panel_exclusive_rect(surface),
        Optional(Eq(geom::Rectangle{{0, 0}, {800, 320}})));
}

TEST_F(InputPanelV1Test, empty_input_region_list_with_zero_content_size_produces_no_exclusive_rect)
{
    surface.set_input_region({});
    surface.resize({0, 0});

    EXPECT_THAT(mf::input_panel_exclusive_rect(surface), Eq(std::nullopt));
}

TEST_F(InputPanelV1Test, clearing_exclusive_rect_sets_outer_spec_optional_with_nullopt_inner)
{
    surface.set_input_region({{{0, 0}, {0, 0}}});

    mir::shell::SurfaceSpecification spec;
    spec.exclusive_rect = mf::input_panel_exclusive_rect(surface);

    ASSERT_TRUE(spec.exclusive_rect.has_value());
    EXPECT_THAT(spec.exclusive_rect.value(), Eq(std::nullopt));
}

TEST_F(InputPanelV1Test, toplevel_spec_sets_output_id_and_south_edge_for_valid_output)
{
    auto const spec = mf::input_panel_toplevel_spec(
        mg::DisplayConfigurationOutputId{42},
        mw::InputPanelSurfaceV1::Position::center_bottom);

    EXPECT_THAT(spec.output_id, Optional(Eq(mg::DisplayConfigurationOutputId{42})));
    EXPECT_THAT(spec.attached_edges, Optional(Eq(mir_placement_gravity_south)));
}

TEST_F(InputPanelV1Test, toplevel_spec_handles_missing_output_id_without_throwing)
{
    auto const spec = mf::input_panel_toplevel_spec(
        std::nullopt,
        mw::InputPanelSurfaceV1::Position::center_bottom);

    EXPECT_THAT(spec.output_id, Eq(std::nullopt));
    EXPECT_THAT(spec.attached_edges, Optional(Eq(mir_placement_gravity_south)));
}

TEST_F(InputPanelV1Test, toplevel_spec_ignores_unknown_position)
{
    auto const spec = mf::input_panel_toplevel_spec(
        mg::DisplayConfigurationOutputId{7},
        99u);

    EXPECT_THAT(spec.output_id, Optional(Eq(mg::DisplayConfigurationOutputId{7})));
    EXPECT_THAT(spec.attached_edges, Eq(std::nullopt));
}
