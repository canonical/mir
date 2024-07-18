/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "static_display_config.h"
#include "mir/logging/dumb_console_logger.h"
#include "mir/logging/logger.h"

#include <mir/test/doubles/mock_display_configuration.h>
#include "mir/test/file_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace mir::geometry;
namespace mg  = mir::graphics;
namespace ml  = mir::logging;
namespace mtd = mir::test::doubles;
using namespace std::string_literals;

namespace
{
static constexpr Point default_top_left{0, 0};
static constexpr mg::DisplayConfigurationMode default_mode{{648, 480}, 60.0};
static constexpr mg::DisplayConfigurationMode another_mode{{1280, 1024}, 75.0};

struct MockLogger : ml::Logger
{
    MOCK_METHOD3(log, void (ml::Severity severity, const std::string& message, const std::string& component));
};

struct StaticDisplayConfig : Test
{
    mtd::MockDisplayConfiguration dc;
    mg::DisplayConfigurationOutput vga1;
    mg::DisplayConfigurationOutput hdmi1;

    miral::YamlFileDisplayConfig sdc;

    std::shared_ptr<MockLogger> const mock_logger{std::make_shared<NiceMock<MockLogger>>()};

    void SetUp() override
    {
        ml::set_logger(mock_logger);
        vga1.id = mg::DisplayConfigurationOutputId{0};
        vga1.card_id = mg::DisplayConfigurationCardId{0};
        vga1.connected = true;
        vga1.type = mg::DisplayConfigurationOutputType::vga;
        vga1.used = true;
        vga1.top_left = default_top_left;
        vga1.preferred_mode_index = 0;
        vga1.current_mode_index = 0;
        vga1.modes.push_back(default_mode);
        vga1.modes.push_back(another_mode);
        vga1.power_mode = mir_power_mode_on;
        vga1.orientation = mir_orientation_normal;
        vga1.name = "VGA-1";

        hdmi1.id = mg::DisplayConfigurationOutputId{1};
        hdmi1.card_id = mg::DisplayConfigurationCardId{0};
        hdmi1.connected = true;
        hdmi1.type = mg::DisplayConfigurationOutputType::hdmia;
        hdmi1.used = true;
        hdmi1.top_left = default_top_left;
        hdmi1.preferred_mode_index = 0;
        hdmi1.current_mode_index = 0;
        hdmi1.modes.push_back(default_mode);
        hdmi1.modes.push_back(another_mode);
        hdmi1.power_mode = mir_power_mode_on;
        hdmi1.orientation = mir_orientation_normal;
        hdmi1.name = "HDMI-A-1";

        EXPECT_CALL(dc, for_each_output(_)).WillRepeatedly(DoAll(
            InvokeArgument<0>(mg::UserDisplayConfigurationOutput{vga1}),
            InvokeArgument<0>(mg::UserDisplayConfigurationOutput{hdmi1})));

        Test::SetUp();
    }

protected:
    void TearDown() override
    {
        Test::TearDown();
        ml::set_logger(std::make_shared<ml::DumbConsoleLogger>());
    }

public:
    static auto constexpr valid_input =
        "layouts:\n"
        "# keys here are layout labels (used for atomically switching between them)\n"
        "# when enabling displays, surfaces should be matched in reverse recency order\n"
        "\n"
        "  default:                       # the first layout is the default\n"
        "\n"
        "    cards:\n"
        "    # a list of cards (currently matched by card-id)\n"
        "\n"
        "    - card-id: 0\n"
        "      VGA-1:\n"
        "        # This output supports the following modes: 1920x1080@60.0, 1680x1050@60.0,\n"
        "        # 1280x1024@75.0, 1280x1024@60.0, 1440x900@59.9, 1280x960@60.0, 1280x800@59.8,\n"
        "        # 1152x864@75.0, 1280x720@60.0, 1024x768@75.0, 1024x768@70.1, 1024x768@60.0,\n"
        "        # 832x624@74.5, 800x600@75.0, 800x600@72.2, 800x600@60.3, 800x600@56.2,\n"
        "        # 640x480@75.0, 640x480@66.7, 640x480@59.9, 720x400@70.1\n"
        "        #\n"
        "        # Uncomment the following to enforce the selected configuration.\n"
        "        # Or amend as desired.\n"
        "        #\n"
        "        state: disabled        # Defaults to preferred enabled\n"
        "        mode: 1280x1024@75.0  # Defaults to preferred mode\n"
        "        # position: [0, 0]      # Defaults to [0, 0]\n"
        "        # orientation: normal   # {normal, left, right, inverted}, Defaults to normal\n"
        "\n"
        "      HDMI-A-1:\n"
        "        # This output supports the following modes: 1920x1080@60.0, 1680x1050@59.9,\n"
        "        # 1280x1024@75.0, 1280x1024@60.0, 1440x900@59.9, 1280x960@60.0, 1280x800@59.9,\n"
        "        # 1152x864@75.0, 1280x720@60.0, 1024x768@75.0, 1024x768@70.1, 1024x768@60.0,\n"
        "        # 832x624@74.5, 800x600@75.0, 800x600@72.2, 800x600@60.3, 800x600@56.2,\n"
        "        # 640x480@75.0, 640x480@66.7, 640x480@59.9, 720x400@70.1\n"
        "        #\n"
        "        # Uncomment the following to enforce the selected configuration.\n"
        "        # Or amend as desired.\n"
        "        #\n"
        "        # state: enabled        # Defaults to preferred enabled\n"
        "        mode: 1280x1024@75.0  # Defaults to preferred mode\n"
        "        # position: [1280, 0]   # Defaults to [0, 0]\n"
        "        orientation: inverted # {normal, left, right, inverted}, Defaults to normal\n"
        "\n"
        "      DisplayPort-1:\n"
        "        # (disconnected)\n"
        "\n"
        "      HDMI-A-2:\n"
        "        # (disconnected)\n"
        "\n"
        "      DisplayPort-2:\n"
        "        # (disconnected)";
};
}

TEST_F(StaticDisplayConfig, nonexistent_config_file_is_no_error)
{
    auto filename = mir::test::create_temp_file();
    unlink(filename.c_str());
    miral::StaticDisplayConfig{filename};
}

TEST_F(StaticDisplayConfig, empty_config_input_causes_AbnormalExit)
{
    std::istringstream empty;

    EXPECT_THROW((sdc.load_config(empty, "")), mir::AbnormalExit);
}

TEST_F(StaticDisplayConfig, ill_formed_config_input_causes_AbnormalExit)
{
    std::istringstream ill_formed{"not YAML = error"};

    EXPECT_THROW((sdc.load_config(ill_formed, "")), mir::AbnormalExit);
}

TEST_F(StaticDisplayConfig, well_formed_non_config_input_causes_AbnormalExit)
{
    std::istringstream well_formed{"well-formed: error"};

    EXPECT_THROW((sdc.load_config(well_formed, "")), mir::AbnormalExit);
}

TEST_F(StaticDisplayConfig, valid_config_input_is_no_error)
{
    std::istringstream valid_config_stream{valid_input};

    sdc.load_config(valid_config_stream, "");
}

TEST_F(StaticDisplayConfig, disabling_vga1_disables_vga1)
{
    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - VGA-1:\n"
        "        state: disabled\n"};

    sdc.load_config(stream, "");

    sdc.apply_to(dc);

    EXPECT_THAT(vga1.used, Eq(false));
    EXPECT_THAT(vga1.power_mode, Eq(mir_power_mode_off));
}

TEST_F(StaticDisplayConfig, sizing_hdmi1_sizes_hdmi1)
{
    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"
        "        mode: 1280x1024\n"};

    sdc.load_config(stream, "");

    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.used, Eq(true));
    EXPECT_THAT(hdmi1.power_mode, Eq(mir_power_mode_on));
    EXPECT_THAT(hdmi1.modes[hdmi1.current_mode_index].size, Eq(Size{1280, 1024}));
}

TEST_F(StaticDisplayConfig, positioning_hdmi1_and_inverting_vga1_works)
{
    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - VGA-1:\n"
        "        orientation: inverted\n"
        "    - HDMI-A-1:\n"
        "        position: [1280, 0]\n"};

    sdc.load_config(stream, "");

    sdc.apply_to(dc);

    EXPECT_THAT(vga1.used, Eq(true));
    EXPECT_THAT(vga1.power_mode, Eq(mir_power_mode_on));
    EXPECT_THAT(vga1.modes[vga1.current_mode_index], Eq(default_mode));
    EXPECT_THAT(vga1.top_left, Eq(Point{0, 0}));
    EXPECT_THAT(vga1.orientation, Eq(mir_orientation_inverted));

    EXPECT_THAT(hdmi1.used, Eq(true));
    EXPECT_THAT(hdmi1.power_mode, Eq(mir_power_mode_on));
    EXPECT_THAT(hdmi1.modes[hdmi1.current_mode_index], Eq(default_mode));
    EXPECT_THAT(hdmi1.top_left, Eq(Point{1280, 0}));
    EXPECT_THAT(hdmi1.orientation, Eq(mir_orientation_normal));
}

TEST_F(StaticDisplayConfig, selecting_default_layout_by_alias_works)
{
    std::istringstream stream{
        "layouts:\n"
        "  another:\n"
        "    cards:\n"
        "    - VGA-1:\n"
        "        state: disabled\n"
        "    - HDMI-A-1:\n"
        "        state: disabled\n"
        "  expected: &my_default\n"
        "    cards:\n"
        "    - VGA-1:\n"
        "        orientation: inverted\n"
        "    - HDMI-A-1:\n"
        "        position: [1280, 0]\n"
        "  default: *my_default\n"};

    sdc.load_config(stream, "");

    sdc.apply_to(dc);

    EXPECT_THAT(vga1.used, Eq(true));
    EXPECT_THAT(vga1.power_mode, Eq(mir_power_mode_on));
    EXPECT_THAT(vga1.modes[vga1.current_mode_index], Eq(default_mode));
    EXPECT_THAT(vga1.top_left, Eq(Point{0, 0}));
    EXPECT_THAT(vga1.orientation, Eq(mir_orientation_inverted));

    EXPECT_THAT(hdmi1.used, Eq(true));
    EXPECT_THAT(hdmi1.power_mode, Eq(mir_power_mode_on));
    EXPECT_THAT(hdmi1.modes[hdmi1.current_mode_index], Eq(default_mode));
    EXPECT_THAT(hdmi1.top_left, Eq(Point{1280, 0}));
    EXPECT_THAT(hdmi1.orientation, Eq(mir_orientation_normal));
}

TEST_F(StaticDisplayConfig, selecting_layout_works)
{
    std::istringstream stream{
        "layouts:\n"
        "  another:\n"
        "    cards:\n"
        "    - VGA-1:\n"
        "        state: disabled\n"
        "    - HDMI-A-1:\n"
        "        state: disabled\n"
        "  expected:\n"
        "    cards:\n"
        "    - VGA-1:\n"
        "        orientation: inverted\n"
        "    - HDMI-A-1:\n"
        "        position: [1280, 0]\n"};

    sdc.load_config(stream, "");
    sdc.select_layout("expected");
    sdc.apply_to(dc);

    EXPECT_THAT(vga1.used, Eq(true));
    EXPECT_THAT(vga1.power_mode, Eq(mir_power_mode_on));
    EXPECT_THAT(vga1.modes[vga1.current_mode_index], Eq(default_mode));
    EXPECT_THAT(vga1.top_left, Eq(Point{0, 0}));
    EXPECT_THAT(vga1.orientation, Eq(mir_orientation_inverted));

    EXPECT_THAT(hdmi1.used, Eq(true));
    EXPECT_THAT(hdmi1.power_mode, Eq(mir_power_mode_on));
    EXPECT_THAT(hdmi1.modes[hdmi1.current_mode_index], Eq(default_mode));
    EXPECT_THAT(hdmi1.top_left, Eq(Point{1280, 0}));
    EXPECT_THAT(hdmi1.orientation, Eq(mir_orientation_normal));
}

TEST_F(StaticDisplayConfig, missing_default_layout_is_reported_and_default_strategy_used)
{
    EXPECT_CALL(*mock_logger, log(Ne(ml::Severity::warning), _, _)).Times(AnyNumber());

    std::istringstream stream{
        "layouts:\n"
        "  unknown:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"};

    EXPECT_CALL(*mock_logger, log(ml::Severity::debug, HasSubstr("Display config using layout strategy: 'default'"), _));

    sdc.load_config(stream, "");
    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.orientation, Eq(mir_orientation_normal));
}

TEST_F(StaticDisplayConfig, missing_side_by_side_layout_is_reported_and_side_by_side_strategy_used)
{
    EXPECT_CALL(*mock_logger, log(Ne(ml::Severity::warning), _, _)).Times(AnyNumber());

    std::istringstream stream{
        "layouts:\n"
        "  unknown:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"};

    EXPECT_CALL(*mock_logger, log(ml::Severity::debug, HasSubstr("Display config using layout strategy: 'side_by_side'"), _));

    sdc.load_config(stream, "");
    sdc.select_layout("side_by_side");

    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.orientation, Eq(mir_orientation_normal));
}

TEST_F(StaticDisplayConfig, missing_foo_layout_is_reported_and_default_strategy_used)
{
    EXPECT_CALL(*mock_logger, log(Ne(ml::Severity::warning), _, _)).Times(AnyNumber());

    std::istringstream stream{
        "layouts:\n"
        "  unknown:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"};

    EXPECT_CALL(*mock_logger, log(ml::Severity::warning, HasSubstr("Display config does not contain layout 'foo'"), _));
    EXPECT_CALL(*mock_logger, log(ml::Severity::debug, HasSubstr("Display config using layout strategy: 'default"), _));

    sdc.load_config(stream, "");
    sdc.select_layout("foo");

    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.orientation, Eq(mir_orientation_normal));
}

TEST_F(StaticDisplayConfig, missing_selected_layout_is_reported_and_ignored)
{
    EXPECT_CALL(*mock_logger, log(Ne(ml::Severity::warning), _, _)).Times(AnyNumber());

    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"};

    EXPECT_CALL(*mock_logger, log(ml::Severity::warning, HasSubstr("unknown"), _));

    sdc.select_layout("unknown");
    sdc.load_config(stream, "");
    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.modes[hdmi1.current_mode_index], Eq(default_mode));
    EXPECT_THAT(hdmi1.orientation, Eq(mir_orientation_normal));
}

TEST_F(StaticDisplayConfig, ill_formed_orientation_causes_AbnormalExit)
{
    std::istringstream ill_formed{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"
        "        orientation: leaft\n"};

    EXPECT_THROW((sdc.load_config(ill_formed, "")), mir::AbnormalExit);
}

TEST_F(StaticDisplayConfig, unknown_mode_is_reported_and_ignored)
{
    EXPECT_CALL(*mock_logger, log(Ne(ml::Severity::warning), _, _)).Times(AnyNumber());

    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"
        "        mode: 11640x480@59.9\n"};

    EXPECT_CALL(*mock_logger, log(ml::Severity::warning, HasSubstr("11640x480@59.9"), _));

    sdc.load_config(stream, "");
    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.modes[hdmi1.current_mode_index], Eq(default_mode));
    EXPECT_THAT(hdmi1.orientation, Eq(mir_orientation_normal));
}

TEST_F(StaticDisplayConfig, ill_formed_status_causes_AbnormalExit)
{
    std::istringstream ill_formed{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"
        "        state: bliss\n"};

    EXPECT_THROW((sdc.load_config(ill_formed, "")), mir::AbnormalExit);
}

TEST_F(StaticDisplayConfig, group_can_be_set)
{
    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"
        "        group: 2\n"};

    sdc.load_config(stream, "");
    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.logical_group_id, Eq(mg::DisplayConfigurationLogicalGroupId{2}));
}

TEST_F(StaticDisplayConfig, given_custom_attributes_when_they_are_not_added_they_are_not_applied)
{
    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"
        "        custom-1: 1\n"
        "        custom-2: two\n"};

    sdc.load_config(stream, "");
    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.custom_attribute, ElementsAre());
}

TEST_F(StaticDisplayConfig, given_custom_attributes_when_they_are_added_they_are_applied)
{
    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"
        "        custom-1: 1\n"
        "        custom-2: two\n"};

    sdc.add_output_attribute("custom-1");
    sdc.add_output_attribute("custom-2");

    sdc.load_config(stream, "");
    sdc.apply_to(dc);

    using KV = decltype(hdmi1.custom_attribute)::value_type;

    EXPECT_THAT(hdmi1.custom_attribute, ElementsAre(KV{"custom-1"s, "1"s}, KV{"custom-2"s, "two"s}));
}

TEST_F(StaticDisplayConfig, given_no_custom_attributes_when_they_are_added_they_are_applied)
{
    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"};

    sdc.add_output_attribute("custom-1");
    sdc.add_output_attribute("custom-2");

    sdc.load_config(stream, "");
    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.custom_attribute, ElementsAre());
}

TEST_F(StaticDisplayConfig, given_no_custom_attributes_when_they_are_added_existing_attributes_are_cleared)
{
    {
        std::istringstream stream{
            "layouts:\n"
            "  default:\n"
            "    cards:\n"
            "    - HDMI-A-1:\n"
            "        custom-1: 1\n"
            "        custom-2: two\n"
        };

        sdc.add_output_attribute("custom-1");
        sdc.add_output_attribute("custom-2");

        sdc.load_config(stream, "");
        sdc.apply_to(dc);
    }

    std::istringstream stream{
        "layouts:\n"
        "  default:\n"
        "    cards:\n"
        "    - HDMI-A-1:\n"};

    sdc.add_output_attribute("custom-1");
    sdc.add_output_attribute("custom-2");

    sdc.load_config(stream, "");
    sdc.apply_to(dc);

    EXPECT_THAT(hdmi1.custom_attribute, ElementsAre());
}
