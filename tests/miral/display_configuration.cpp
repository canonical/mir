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

#include "miral/display_configuration.h"
#include "miral/test_server.h"

#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace miral;
using namespace testing;
namespace mt = mir::test;

struct BaseTestDisplayConfiguration : TestDisplayServer, Test
{
    void SetUp() override
    {
        // Set environment variables such that we'll always read the display
        // configuration from the temporary directory.
        setenv("XDG_CONFIG_HOME", "/tmp", 1);
        unsetenv("XDG_CONFIG_DIRS");
        unsetenv("HOME");

        invoke_runner([&](MirRunner const& runner)
        {
            configuration = std::make_shared<DisplayConfiguration>(runner);

            add_server_init([&](mir::Server& server)
            {
                before_read("/tmp/" + runner.display_config_file());
                (*configuration)(server);
            });
        });

        start_server();
    }

    void TearDown() override
    {
        stop_server();
    }

    /// Called before the display configuration is read. Implementers should
    /// use this method to write the yaml file that they would like to test
    /// and set up any callbacks that they expect to be triggered as a result
    /// of reading.
    virtual void before_read(std::string const& filepath) = 0;

    std::shared_ptr<DisplayConfiguration> configuration;
};

struct TestCustomLayoutKeysDisplayConfiguration : BaseTestDisplayConfiguration
{
    void before_read(std::string const& filepath) override
    {
        // First, write the yaml payload to the file
        std::ofstream file(filepath);
        std::string yaml{
            "layouts:\n"
            "  first:\n"
            "    cards:\n"
            "    - VGA-1:\n"
            "        state: disabled\n"
            "    - HDMI-A-1:\n"
            "        state: disabled\n"
            "    custom_string: value\n"
            "    custom_int: 10\n"
            "    custom_sequence:\n"
            "        - first\n"
            "    custom_map:\n"
            "        key: value\n"
            "  second:\n"
            "    cards:\n"
            "    - VGA-1:\n"
            "        orientation: inverted\n"
            "    - HDMI-A-1:\n"
            "        position: [1280, 0]\n"};
        file << yaml;

        // Next, establish all of the expected keys
        configuration->layout_userdata_builder("custom_string", [](DisplayConfiguration::Node const& node)
        {
            EXPECT_THAT(node.type(), Eq(miral::DisplayConfiguration::Node::Type::string));
            return node.as_string();
        });
        configuration->layout_userdata_builder("custom_int", [](DisplayConfiguration::Node const& node)
        {
            EXPECT_THAT(node.type(), Eq(miral::DisplayConfiguration::Node::Type::integer));
            return node.as_int();
        });
        configuration->layout_userdata_builder("custom_sequence", [](DisplayConfiguration::Node const& node)
        {
            EXPECT_THAT(node.type(), Eq(miral::DisplayConfiguration::Node::Type::sequence));
            std::string value;
            node.for_each([&](DisplayConfiguration::Node const& sub_node)
            {
                value = sub_node.as_string();
            });
            return value;
        });
        configuration->layout_userdata_builder("custom_map", [](DisplayConfiguration::Node const& node)
        {
            EXPECT_THAT(node.type(), Eq(miral::DisplayConfiguration::Node::Type::map));
            EXPECT_TRUE(node.has("key"));
            return node.at("key").value().as_string();
        });
    }
};

TEST_F(TestCustomLayoutKeysDisplayConfiguration, can_read_custom_keys_of_all_types_for_a_layout)
{
    configuration->select_layout("first");
    EXPECT_THAT(std::any_cast<std::string>(configuration->layout_userdata("custom_string").value()), Eq("value"));
    EXPECT_THAT(std::any_cast<int>(configuration->layout_userdata("custom_int").value()), Eq(10));
    EXPECT_THAT(std::any_cast<std::string>(configuration->layout_userdata("custom_sequence").value()), Eq("first"));
    EXPECT_THAT(std::any_cast<std::string>(configuration->layout_userdata("custom_map").value()), Eq("value"));
}

TEST_F(TestCustomLayoutKeysDisplayConfiguration, unseen_keys_are_set_to_nullopt)
{
    configuration->select_layout("second");
    EXPECT_THAT(configuration->layout_userdata("custom_string").has_value(), Eq(false));
    EXPECT_THAT(configuration->layout_userdata("custom_int").has_value(), Eq(false));
    EXPECT_THAT(configuration->layout_userdata("custom_sequence").has_value(), Eq(false));
    EXPECT_THAT(configuration->layout_userdata("custom_map").has_value(), Eq(false));
}
