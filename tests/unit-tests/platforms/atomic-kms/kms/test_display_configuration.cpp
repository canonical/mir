/*
 * Copyright © Canonical Ltd.
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
 */

#include "src/platforms/atomic-kms/server/kms/real_kms_display_configuration.h"
#include "src/platforms/atomic-kms/server/platform_common.h"

#include <mir/test/doubles/stub_display_configuration.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace mg = mir::graphics;
namespace mga = mir::graphics::atomic;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{

geom::Size const mode_size{1920, 1080};
mg::DisplayConfigurationMode const native_mode{mode_size, 60.0};
std::vector<MirPixelFormat> const pixel_formats{mir_pixel_format_xrgb_8888};

mg::DisplayConfigurationOutputId const display_1{1};
mg::DisplayConfigurationOutputId const display_2{2};
mg::DisplayConfigurationOutputId const display_3{3};

geom::Point const left_display_origin{0, 0};
geom::Point const right_display_origin{geom::as_x(mode_size.width), 0};

auto active(mg::DisplayConfigurationOutputId id, geom::Point top_left = {}) -> mg::DisplayConfigurationOutput
{
    mg::DisplayConfigurationOutput output = mtd::StubDisplayConfigurationOutput{id, {native_mode}, pixel_formats};
    output.top_left = top_left;
    return output;
}

auto disconnected(mg::DisplayConfigurationOutputId id) -> mg::DisplayConfigurationOutput
{
    auto output = active(id);
    output.connected = false;
    return output;
}

auto unused(mg::DisplayConfigurationOutputId id) -> mg::DisplayConfigurationOutput
{
    auto output = active(id);
    output.used = false;
    return output;
}

auto powered(mg::DisplayConfigurationOutputId id, MirPowerMode power_mode) -> mg::DisplayConfigurationOutput
{
    auto output = active(id);
    output.power_mode = power_mode;
    return output;
}

auto without_mode(mg::DisplayConfigurationOutputId id) -> mg::DisplayConfigurationOutput
{
    auto output = active(id);
    output.current_mode_index = static_cast<uint32_t>(output.modes.size());
    return output;
}

auto clone_of(mg::DisplayConfigurationOutput const& output, mg::DisplayConfigurationOutputId id)
    -> mg::DisplayConfigurationOutput
{
    auto clone = output;
    clone.id = id;
    return clone;
}

struct PlanePromotionCase
{
    std::string name;
    std::vector<mg::DisplayConfigurationOutput> outputs;
    mga::PlanePromotion expected;
};

class PlanePromotionFor : public TestWithParam<PlanePromotionCase>
{
};
}

TEST_P(PlanePromotionFor, is_available_iff_exactly_one_output_is_active)
{
    auto const& param = GetParam();
    mtd::StubDisplayConfig const configuration{param.outputs};

    EXPECT_EQ(param.expected, mga::plane_promotion_for(configuration));
}

INSTANTIATE_TEST_SUITE_P(
    DisplayConfiguration,
    PlanePromotionFor,
    Values(
        PlanePromotionCase{"no_outputs", {}, mga::PlanePromotion::unavailable},
        PlanePromotionCase{"one_active", {active(display_1)}, mga::PlanePromotion::available},
        PlanePromotionCase{
            "one_active_and_one_disconnected",
            {active(display_1), disconnected(display_2)},
            mga::PlanePromotion::available},
        PlanePromotionCase{
            "one_active_and_one_unused",
            {active(display_1), unused(display_2)},
            mga::PlanePromotion::available},
        PlanePromotionCase{
            "one_active_and_one_powered_off",
            {active(display_1), powered(display_2, mir_power_mode_off)},
            mga::PlanePromotion::available},
        PlanePromotionCase{
            "one_active_and_one_in_standby",
            {active(display_1), powered(display_2, mir_power_mode_standby)},
            mga::PlanePromotion::available},
        PlanePromotionCase{
            "one_active_and_one_without_a_valid_mode",
            {active(display_1), without_mode(display_2)},
            mga::PlanePromotion::available},
        PlanePromotionCase{
            "only_inactive_outputs",
            {disconnected(display_1), unused(display_2), powered(display_3, mir_power_mode_off)},
            mga::PlanePromotion::unavailable},
        PlanePromotionCase{
            "two_active_extended",
            {active(display_1, left_display_origin), active(display_2, right_display_origin)},
            mga::PlanePromotion::unavailable},
        PlanePromotionCase{
            "two_active_clones",
            {active(display_1, left_display_origin), clone_of(active(display_1, left_display_origin), display_2)},
            mga::PlanePromotion::unavailable}),
    [](TestParamInfo<PlanePromotionCase> const& info) { return info.param.name; });

TEST(PlanePromotion, each_device_configuration_is_decided_independently)
{
    std::vector<mg::DisplayConfigurationOutput> const single_output{active(display_1)};
    std::vector<mg::DisplayConfigurationOutput> const two_outputs{active(display_1), active(display_2)};

    mtd::StubDisplayConfig const device_a{single_output};
    mtd::StubDisplayConfig device_b{single_output};

    EXPECT_EQ(mga::PlanePromotion::available, mga::plane_promotion_for(device_a));
    EXPECT_EQ(mga::PlanePromotion::available, mga::plane_promotion_for(device_b));

    device_b.outputs = two_outputs;

    EXPECT_EQ(mga::PlanePromotion::available, mga::plane_promotion_for(device_a));
    EXPECT_EQ(mga::PlanePromotion::unavailable, mga::plane_promotion_for(device_b));
}
