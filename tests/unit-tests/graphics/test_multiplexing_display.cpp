/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir/graphics/display_configuration.h"
#include "mir/test/doubles/mock_display.h"
#include "mir/test/doubles/stub_display_configuration.h"

#include "mir_toolkit/common.h"
#include "src/server/graphics/multiplexing_display.h"


namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

class DisplayConfigurationOutputGenerator
{
public:
    DisplayConfigurationOutputGenerator(int card_id)
        : card{mg::DisplayConfigurationCardId{card_id}},
          next_id{23}
    {
    }

    auto generate_output()
        -> mg::DisplayConfigurationOutput
    {
        return mg::DisplayConfigurationOutput {
            next_output_id(),
            card,
            mg::DisplayConfigurationLogicalGroupId{1},
            mg::DisplayConfigurationOutputType::edp,
            {
                mir_pixel_format_argb_8888,
                mir_pixel_format_xrgb_8888
            },
            {
                {geom::Size{3840, 2160}, 59.98},
            },
            0,
            geom::Size{340, 190},
            true,
            true,
            geom::Point{0,0},
            0,
            mir_pixel_format_xrgb_8888,
            mir_power_mode_on,
            mir_orientation_normal,
            2.0f,
            mir_form_factor_monitor,
            mir_subpixel_arrangement_horizontal_rgb,
            {},
            mir_output_gamma_unsupported,
            {},
            {}
        };
    }

private:
    auto next_output_id() -> mg::DisplayConfigurationOutputId
    {
        return mg::DisplayConfigurationOutputId{++next_id};
    }

    mg::DisplayConfigurationCardId const card;
    int next_id;
};
}

TEST(MultiplexingDisplay, forwards_for_each_display_sync_group)
{
    using namespace testing;

    std::vector<std::unique_ptr<mg::Display>> displays;

    for (int i = 0u; i < 2; ++i)
    {
        auto mock_display = std::make_unique<NiceMock<mtd::MockDisplay>>();
        EXPECT_CALL(*mock_display, for_each_display_sync_group(_)).Times(1);
        displays.push_back(std::move(mock_display));
    }

    mg::MultiplexingDisplay display{std::move(displays)};

    display.for_each_display_sync_group([](auto const&) {});
}

TEST(MultiplexingDisplay, configuration_is_union_of_all_displays)
{
    using namespace testing;

    std::vector<mg::DisplayConfigurationOutput> first_display_conf, second_display_conf, third_display_conf;

    DisplayConfigurationOutputGenerator card_one{1}, card_two{3}, card_three{42};

    for (auto i = 0u; i < 3; ++i)
    {
        first_display_conf.push_back(card_one.generate_output());
    }
    for (auto i = 0u; i < 4; ++i)
    {
        second_display_conf.push_back(card_two.generate_output());
    }
    for (auto i = 0u; i < 2; ++i)
    {
        third_display_conf.push_back(card_three.generate_output());
    }

    std::vector<std::unique_ptr<mg::Display>> displays;

    for (auto const& conf : {first_display_conf, second_display_conf, third_display_conf})
    {
        auto display = std::make_unique<NiceMock<mtd::MockDisplay>>();
        ON_CALL(*display, configuration())
            .WillByDefault(
                Invoke(
                    [conf]()
                    {
                        return std::make_unique<mtd::StubDisplayConfig>(conf);
                    }));

        displays.push_back(std::move(display));
    }

    mg::MultiplexingDisplay display{std::move(displays)};

    auto conf = display.configuration();

    std::vector<mg::DisplayConfigurationOutput> visible_conf;
    conf->for_each_output(
        [&visible_conf](mg::DisplayConfigurationOutput const& conf)
        {
            visible_conf.push_back(conf);
        });

    std::vector<mg::DisplayConfigurationOutput> expected_outputs;
    // Deliberately insert these out of order, to ensure the test doesn't mistakenly enforce ordering
    expected_outputs.insert(expected_outputs.end(), second_display_conf.begin(), second_display_conf.end());
    expected_outputs.insert(expected_outputs.end(), first_display_conf.begin(), first_display_conf.end());
    expected_outputs.insert(expected_outputs.end(), third_display_conf.begin(), third_display_conf.end());
    EXPECT_THAT(visible_conf, UnorderedElementsAreArray(expected_outputs));
}

TEST(MultiplexingDisplay, dispatches_configure_to_associated_platform)
{
    using namespace testing;

    DisplayConfigurationOutputGenerator gen1{5}, gen2{42};

    auto d1 = std::make_unique<NiceMock<mtd::MockDisplay>>();
    auto d2 = std::make_unique<NiceMock<mtd::MockDisplay>>();

    auto conf1 = std::make_unique<mtd::StubDisplayConfig>(
        std::vector<mg::DisplayConfigurationOutput>{ gen1.generate_output(), gen1.generate_output() });
    auto conf2 = std::make_unique<mtd::StubDisplayConfig>(
        std::vector<mg::DisplayConfigurationOutput>{ gen2.generate_output() });

    // Each Display should get a configure() call with only its own configuration
    EXPECT_CALL(*d1, configure(Address(conf1.get())));
    EXPECT_CALL(*d2, configure(Address(conf2.get())));

    EXPECT_CALL(*d1, configuration())
        .WillOnce(Invoke([&conf1]() mutable { return std::move(conf1); }));
    EXPECT_CALL(*d2, configuration())
        .WillOnce(Invoke([&conf2]() mutable { return std::move(conf2); }));

    std::vector<std::unique_ptr<mg::Display>> displays;
    displays.push_back(std::move(d1));
    displays.push_back(std::move(d2));

    mg::MultiplexingDisplay display{std::move(displays)};
    auto conf = display.configuration();

    display.configure(*conf);
}

MATCHER_P(IsConfigurationOfCard, cardid, "")
{
    bool matches{true};
    arg.for_each_output(
        [&matches, this](auto const& output)
        {
            matches &= output.card_id == mg::DisplayConfigurationCardId{cardid};
        });
    return matches;
}

TEST(MultiplexingDisplay, apply_if_confguration_preserves_display_buffers_succeeds_if_all_succeed)
{
    using namespace testing;

    DisplayConfigurationOutputGenerator gen1{5}, gen2{42};

    auto d1 = std::make_unique<NiceMock<mtd::MockDisplay>>();
    auto d2 = std::make_unique<NiceMock<mtd::MockDisplay>>();

    ON_CALL(*d1, configuration())
        .WillByDefault(
            Invoke(
                [&gen1]()
                {
                    return std::make_unique<mtd::StubDisplayConfig>(
                        std::vector<mg::DisplayConfigurationOutput>{ gen1.generate_output(), gen1.generate_output() });
                 }));
    ON_CALL(*d2, configuration())
        .WillByDefault(
            Invoke(
                [&gen2]()
                {
                    return std::make_unique<mtd::StubDisplayConfig>(
                        std::vector<mg::DisplayConfigurationOutput>{ gen2.generate_output() });
                 }));

    // Each Display should get a configure() call with only its own configuration
    EXPECT_CALL(*d1, apply_if_configuration_preserves_display_buffers(IsConfigurationOfCard(5)))
        .WillOnce(Return(true));
    EXPECT_CALL(*d2, apply_if_configuration_preserves_display_buffers(IsConfigurationOfCard(42)))
        .WillOnce(Return(true));

    std::vector<std::unique_ptr<mg::Display>> displays;
    displays.push_back(std::move(d1));
    displays.push_back(std::move(d2));

    mg::MultiplexingDisplay display{std::move(displays)};
    auto conf = display.configuration();

    display.apply_if_configuration_preserves_display_buffers(*conf);
}

TEST(MultiplexingDisplay, apply_if_confguration_preserves_display_buffers_fails_if_any_fail)
{
    using namespace testing;

    DisplayConfigurationOutputGenerator gen1{5}, gen2{42};

    auto d1 = std::make_unique<NiceMock<mtd::MockDisplay>>();
    auto d2 = std::make_unique<NiceMock<mtd::MockDisplay>>();

    ON_CALL(*d1, configuration())
        .WillByDefault(
            Invoke(
                [&gen1]()
                {
                    return std::make_unique<mtd::StubDisplayConfig>(
                        std::vector<mg::DisplayConfigurationOutput>{ gen1.generate_output(), gen1.generate_output() });
                 }));
    ON_CALL(*d2, configuration())
        .WillByDefault(
            Invoke(
                [&gen2]()
                {
                    return std::make_unique<mtd::StubDisplayConfig>(
                        std::vector<mg::DisplayConfigurationOutput>{ gen2.generate_output() });
                 }));

    // Each Display should get a configure() call with only its own configuration
    EXPECT_CALL(*d1, apply_if_configuration_preserves_display_buffers(IsConfigurationOfCard(5)))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*d2, apply_if_configuration_preserves_display_buffers(IsConfigurationOfCard(42)))
        .WillOnce(Return(false));

    std::vector<std::unique_ptr<mg::Display>> displays;
    displays.push_back(std::move(d1));
    displays.push_back(std::move(d2));

    mg::MultiplexingDisplay display{std::move(displays)};
    auto conf = display.configuration();

    EXPECT_THAT(display.apply_if_configuration_preserves_display_buffers(*conf), Eq(false));
}

TEST(MultiplexingDisplay, apply_if_configuration_preserves_display_buffers_fails_to_previous_configuration)
{
    using namespace testing;

    DisplayConfigurationOutputGenerator gen1{5}, gen2{42};

    auto d1 = std::make_unique<NiceMock<mtd::MockDisplay>>();
    auto d2 = std::make_unique<NiceMock<mtd::MockDisplay>>();

    auto d1_conf = std::make_unique<mtd::StubDisplayConfig>(
        std::vector<mg::DisplayConfigurationOutput>{
            gen1.generate_output(),
            gen1.generate_output()
        });

    auto d2_conf = std::make_unique<mtd::StubDisplayConfig>(
        std::vector<mg::DisplayConfigurationOutput>{
            gen2.generate_output(),
            gen2.generate_output(),
            gen2.generate_output()
        });

    ON_CALL(*d1, configuration())
        .WillByDefault(
            Invoke(
                [&d1_conf]()
                {
                    return d1_conf->clone();
                }));
    ON_CALL(*d2, configuration())
        .WillByDefault(
            Invoke(
                [&d2_conf]()
                {
                    return d2_conf->clone();
                }));

    // Each Display should get a configure() call with only its own configuration
    EXPECT_CALL(*d1, apply_if_configuration_preserves_display_buffers(IsConfigurationOfCard(5)))
        .WillRepeatedly(
            Invoke(
                [&d1_conf](auto const& config)
                {
                    auto real_config = dynamic_cast<mtd::StubDisplayConfig const&>(config);
                    d1_conf->outputs = real_config.outputs;
                    return true;
                }));
    EXPECT_CALL(*d2, apply_if_configuration_preserves_display_buffers(IsConfigurationOfCard(42)))
        .WillOnce(Return(false));

    std::vector<std::unique_ptr<mg::Display>> displays;
    displays.push_back(std::move(d1));
    displays.push_back(std::move(d2));

    mg::MultiplexingDisplay display{std::move(displays)};
    auto preexisting_conf = display.configuration();
    auto changed_conf = display.configuration();

    changed_conf->for_each_output(
        [](mg::UserDisplayConfigurationOutput& output)
        {
            output.orientation = mir_orientation_inverted;
        });

    ASSERT_THAT(display.apply_if_configuration_preserves_display_buffers(*changed_conf), Eq(false));
    auto new_conf = display.configuration();

    EXPECT_THAT(*new_conf, Eq(std::cref(*preexisting_conf)));    // We need std::cref() to stop GTest trying to treat
                                                                 // preexisting_conf as a(n impossible) value rather than reference
}
