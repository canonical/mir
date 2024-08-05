/*
 * Copyright © Canonical Ltd.
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

#include "mir_test_framework/headless_test.h"

#include "mir/options/option.h"

#include "mir/test/file_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdlib>
#include <fstream>
#include <system_error>

using namespace testing;

namespace
{
struct ServerConfigurationOptions : mir_test_framework::HeadlessTest
{
    MOCK_METHOD(void, command_line_handler, (std::vector<std::string> const&));

    void SetUp() override
    {
        server.set_command_line_handler([this](int argc, char const* const* argv)
            {
                std::vector<std::string> args;
                for (auto p = argv; p != argv+argc; ++p)
                {
                    args.emplace_back(*p);
                }
                command_line_handler(args);
            });

        server.add_configuration_option(test_config_key, "", mir::OptionType::string);

        add_to_environment(env_xdg_config_home, fake_xdg_config_home.c_str());
        add_to_environment(env_home, fake_home.c_str());
        add_to_environment(env_xdg_config_dirs, fake_xdg_config_dirs.c_str());
    }

    void TearDown() override
    {
        // In the event of failure leave test files around to aid debugging
        if(!HasFailure())
            clean_test_files();
    }

    static constexpr char const* const env_xdg_config_home = "XDG_CONFIG_HOME";
    static constexpr char const* const env_home = "HOME";
    static constexpr char const* const env_xdg_config_dirs = "XDG_CONFIG_DIRS";

    static constexpr char const* const not_found = "not found";
    std::string const config_filename{"test.config"};
    static constexpr char const* const test_config_key = "config_dir";

    void create_config_file_in(std::string const& dir)
    {
        mkdir(dir.c_str(), 0700);

        auto const filename = dir + ('/' + config_filename);

        std::ofstream config(filename);
        config << test_config_key << '=' << dir << std::endl;
    }

    void remove_config_file_in(std::string const& dir)
    {
        remove((dir + ('/' + config_filename)).c_str());
        remove(dir.c_str());
    }

    void clean_test_files()
    {
        remove_config_file_in(fake_xdg_config_dir0);
        remove_config_file_in(fake_xdg_config_dir1);
        remove_config_file_in(fake_xdg_config_home);
        remove_config_file_in(fake_home_config);
        remove(fake_home.c_str());
        remove(temp_dir.c_str());
    }

    std::string const temp_dir = mir::test::create_temp_dir();
    std::string const fake_xdg_config_home = temp_dir + "/fake_xdg_config_home";
    std::string const fake_home = temp_dir + "/fake_home";
    std::string const fake_home_config = temp_dir + "/fake_home/.config";
    std::string const fake_xdg_config_dir0 = temp_dir + "/fake_xdg_config_dir0";
    std::string const fake_xdg_config_dir1 = temp_dir + "/fake_xdg_config_dir1";
    std::string const fake_xdg_config_dirs =
        fake_xdg_config_dir0 + ":" + fake_xdg_config_dir1;
};
}

TEST_F(ServerConfigurationOptions, unknown_command_line_options_are_passed_to_handler)
{
    const int argc = 7;
    char const* argv[argc] = {
        __PRETTY_FUNCTION__,
        "--enable-input", "no",
        "--hello",
        "world",
        "--answer", "42"
    };

    server.set_command_line(argc, argv);

    EXPECT_CALL(*this, command_line_handler(
        ElementsAre(StrEq("--hello"), StrEq("world"), StrEq("--answer"), StrEq("42"))));

    server.apply_settings();
}

TEST_F(ServerConfigurationOptions, are_read_from_xdg_config_home)
{
    create_config_file_in(fake_xdg_config_home);

    server.set_config_filename(config_filename);
    server.apply_settings();

    auto const options = server.get_options();

    EXPECT_THAT(options->get(test_config_key, not_found), StrEq(fake_xdg_config_home));
}

TEST_F(ServerConfigurationOptions, are_read_from_home_config_file)
{
    mkdir(fake_home.c_str(), 0700);
    create_config_file_in(fake_home_config);

    // $HOME is only used if $XDG_CONFIG_HOME isn't set
    add_to_environment(env_xdg_config_home, nullptr);

    server.set_config_filename(config_filename);
    server.apply_settings();

    auto const options = server.get_options();

    EXPECT_THAT(options->get(test_config_key, not_found), StrEq(fake_home_config));
}

TEST_F(ServerConfigurationOptions, are_read_from_xdg_config_dir0_config_file)
{
    create_config_file_in(fake_xdg_config_dir0);

    server.set_config_filename(config_filename);
    server.apply_settings();

    auto const options = server.get_options();

    EXPECT_THAT(options->get(test_config_key, not_found), StrEq(fake_xdg_config_dir0));
}

TEST_F(ServerConfigurationOptions, are_read_from_xdg_config_dir1_config_file)
{
    create_config_file_in(fake_xdg_config_dir1);

    server.set_config_filename(config_filename);
    server.apply_settings();

    auto const options = server.get_options();

    EXPECT_THAT(options->get(test_config_key, not_found), StrEq(fake_xdg_config_dir1));
}

TEST_F(ServerConfigurationOptions, are_read_from_xdg_config_dir0_before_xdg_config_dir1)
{
    create_config_file_in(fake_xdg_config_dir0);
    create_config_file_in(fake_xdg_config_dir1);

    server.set_config_filename(config_filename);
    server.apply_settings();

    auto const options = server.get_options();

    EXPECT_THAT(options->get(test_config_key, not_found), StrEq(fake_xdg_config_dir0));
}

TEST_F(ServerConfigurationOptions, are_read_from_xdg_config_home_before_xdg_config_dirs)
{
    create_config_file_in(fake_xdg_config_home);
    create_config_file_in(fake_xdg_config_dir0);
    create_config_file_in(fake_xdg_config_dir1);

    server.set_config_filename(config_filename);
    server.apply_settings();

    auto const options = server.get_options();

    EXPECT_THAT(options->get(test_config_key, not_found), StrEq(fake_xdg_config_home));
}
