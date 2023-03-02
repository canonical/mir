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

#include <boost/throw_exception.hpp>

#include "mir/graphics/display_configuration.h"
#include "mir/test/doubles/mock_display.h"
#include "mir/test/doubles/mock_main_loop.h"
#include "mir/test/doubles/stub_display_configuration.h"

#include "mir_toolkit/common.h"
#include "src/server/graphics/multiplexing_display.h"


namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

using namespace testing;

namespace
{

class DisplayConfigurationOutputGenerator
{
public:
    DisplayConfigurationOutputGenerator(int card_id)
        : card{mg::DisplayConfigurationCardId{card_id}},
          next_id{1}
    {
    }

    auto generate_output()
        -> mg::DisplayConfigurationOutput
    {
        auto const output_id = next_output_id();
        return mg::DisplayConfigurationOutput {
            output_id,
            card,
            mg::DisplayConfigurationLogicalGroupId{1},
            mg::DisplayConfigurationOutputType::edp,
            pixel_formats_for_output_id(output_id),
            {
                {geom::Size{3840, 2160}, 59.98},
            },
            0,
            geom::Size{340, 190},
            true,
            true,
            geom::Point{0,0},
            0,
            pixel_formats_for_output_id(output_id)[0],
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
    auto pixel_formats_for_output_id(mg::DisplayConfigurationOutputId id)
        -> std::vector<MirPixelFormat>
    {
        std::vector<MirPixelFormat> distinguishing_formats;

        int remainder = id.as_value();
        int minimum_choice = 1;
        while (remainder > 0)
        {
            int num_current_choices = mir_pixel_formats - minimum_choice;
            if (num_current_choices == 0)
            {
                BOOST_THROW_EXCEPTION((std::runtime_error{"Attempted to create more distinguishable DisplayConfigurations than curretly possible"}));
            }
            auto choice = remainder % num_current_choices;
            if (choice > 0)
            {
                distinguishing_formats.push_back(static_cast<MirPixelFormat>(choice));
            }
            remainder = remainder / num_current_choices;
            num_current_choices--;
        }
        return distinguishing_formats;
    }

    auto next_output_id() -> mg::DisplayConfigurationOutputId
    {
        return mg::DisplayConfigurationOutputId{++next_id};
    }

    mg::DisplayConfigurationCardId const card;
    int next_id;
};

MATCHER_P(OutputConfigurationEqualIgnoringId, output, "")
{
    mg::DisplayConfigurationOutput arg_copy = arg;
    arg_copy.id = output.id;
    return ExplainMatchResult(
        Eq(output),
        arg_copy,
        result_listener);
}
}

TEST(MultiplexingDisplay, forwards_for_each_display_sync_group)
{
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

TEST(MultiplexingDisplay, each_display_in_configuration_has_unique_id)
{
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

    std::vector<mg::DisplayConfigurationOutputId> seen_ids;
    conf->for_each_output(
        [&seen_ids](mg::UserDisplayConfigurationOutput const& conf)
        {
            EXPECT_THAT(seen_ids, Not(Contains(conf.id)));
            seen_ids.push_back(conf.id);
        });
}

TEST(MultiplexingDisplay, configuration_is_union_of_all_displays)
{
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

    std::vector<Matcher<mg::DisplayConfigurationOutput>> expected_outputs;
    // Deliberately insert these out of order, to ensure the test doesn't mistakenly enforce ordering
    std::transform(
        second_display_conf.begin(), second_display_conf.end(),
        std::back_inserter(expected_outputs),
        [](auto const& output) { return OutputConfigurationEqualIgnoringId(output); });
    std::transform(
        first_display_conf.begin(), first_display_conf.end(),
        std::back_inserter(expected_outputs),
        [](auto const& output) { return OutputConfigurationEqualIgnoringId(output); });
    std::transform(
        third_display_conf.begin(), third_display_conf.end(),
        std::back_inserter(expected_outputs),
        [](auto const& output) { return OutputConfigurationEqualIgnoringId(output); });

    EXPECT_THAT(visible_conf, UnorderedElementsAreArray(expected_outputs));
}

TEST(MultiplexingDisplay, dispatches_configure_to_associated_platform)
{
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

TEST(MultiplexingDisplay, apply_if_configuration_preserves_display_buffers_throws_on_unrecoverable_error)
{
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

    // The first display will get two configuration requests…
    EXPECT_CALL(*d1, apply_if_configuration_preserves_display_buffers(IsConfigurationOfCard(5)))
        .WillOnce(
            Invoke(
                [&d1_conf](auto const& config)
                {
                    auto real_config = dynamic_cast<mtd::StubDisplayConfig const&>(config);
                    d1_conf->outputs = real_config.outputs;
                    return true;        // The first will succeed, changing state…
                }))
        .WillOnce(
            Return(false));            // The second (to return to initial configuration) will fail
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

    EXPECT_THROW(
        { display.apply_if_configuration_preserves_display_buffers(*changed_conf); },
        mg::Display::IncompleteConfigurationApplied);
}

TEST(MultiplexingDisplay, delegates_registering_configuration_change_handlers)
{
    std::vector<std::unique_ptr<mg::Display>> mock_displays;

    for (auto i = 0; i < 5 ; ++i)
    {
        auto mock_display = std::make_unique<NiceMock<mtd::MockDisplay>>();
        EXPECT_CALL(*mock_display, register_configuration_change_handler(_,_));
        mock_displays.push_back(std::move(mock_display));
    }

    mg::MultiplexingDisplay display{std::move(mock_displays)};

    mtd::MockMainLoop ml;
    display.register_configuration_change_handler(ml, [](){});
}

TEST(MultiplexingDisplay, delegates_last_frame_on_to_appropriate_platform)
{
    DisplayConfigurationOutputGenerator gen1{5}, gen2{42};

    auto d1 = std::make_unique<NiceMock<mtd::MockDisplay>>();
    auto d2 = std::make_unique<NiceMock<mtd::MockDisplay>>();

    auto d1_conf = std::make_unique<mtd::StubDisplayConfig>(
        std::vector<mg::DisplayConfigurationOutput>{
            gen1.generate_output(),
            gen1.generate_output(),
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

    // Each output can expect to get exactly one call.
    d1->configuration()->for_each_output(
        [&d1](mg::DisplayConfigurationOutput const& output)
        {
            EXPECT_CALL(*d1, last_frame_on(Eq(output.id.as_value()))).Times(1);
        });
    d2->configuration()->for_each_output(
        [&d2](mg::DisplayConfigurationOutput const& output)
        {
            EXPECT_CALL(*d2, last_frame_on(Eq(output.id.as_value()))).Times(1);
        });

    std::vector<std::unique_ptr<mg::Display>> displays;
    displays.push_back(std::move(d1));
    displays.push_back(std::move(d2));

    mg::MultiplexingDisplay display{std::move(displays)};
    display.configuration()->for_each_output(
        [&display](mg::UserDisplayConfigurationOutput& output)
        {
            display.last_frame_on(output.id.as_value());
        });
}
