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

#include <mir_test_framework/headless_test.h>

#include <mir/abnormal_exit.h>
#include <mir/options/option.h>

#include <boost/program_options/errors.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdlib>
#include <fstream>
#include <system_error>

using namespace testing;

namespace
{
std::string create_temp_dir()
{
    char temp_dir[] = "temp_dir_XXXXXX";
    if (mkdtemp(temp_dir) == nullptr)
        throw std::system_error(errno, std::system_category(), "Failed to create temp dir");

    return temp_dir;
}

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

    std::string const temp_dir = create_temp_dir();
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

TEST_F(ServerConfigurationOptions, are_parsed_from_environment)
{
    auto const option_name = "test-option";
    auto const option_env_key = "MIR_SERVER_TEST_OPTION";
    auto const option_env_value = "a string";

    add_to_environment(option_env_key, option_env_value);

    server.add_configuration_option(option_name, "", mir::OptionType::string);

    server.apply_settings();

    auto const options = server.get_options();
    EXPECT_THAT(options->get<std::string>(option_name), StrEq(option_env_value));
}

TEST_F(ServerConfigurationOptions, unknown_environment_options_are_fatal)
{
    auto const option_env_key = "MIR_SERVER_NOT_ACTUALLY_AN_OPTION_DEFINED";
    auto const option_env_value = "a string";

    add_to_environment(option_env_key, option_env_value);

    EXPECT_THROW(
        {
            server.apply_settings();

            server.get_options();
        },
        boost::program_options::unknown_option
    );
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

TEST_F(ServerConfigurationOptions, string_default_can_be_overridden)
{
    server.set_configuration_option_default("cursor", "null");

    server.apply_settings();

    auto const options = server.get_options();
    EXPECT_THAT(options->get<std::string>("cursor"), StrEq("null"));
}

TEST_F(ServerConfigurationOptions, int_default_can_be_overridden)
{
    server.set_configuration_option_default("composite-delay", 42);

    server.apply_settings();

    auto const options = server.get_options();
    EXPECT_THAT(options->get<int>("composite-delay"), Eq(42));
}

TEST_F(ServerConfigurationOptions, bool_default_can_be_overridden)
{
    server.set_configuration_option_default("enable-input", false);

    server.apply_settings();

    auto const options = server.get_options();
    EXPECT_FALSE(options->get<bool>("enable-input"));
}

TEST_F(ServerConfigurationOptions, command_line_takes_precedence_over_overridden_default)
{
    const int argc = 3;
    char const* argv[argc] = {__PRETTY_FUNCTION__, "--cursor", "software"};

    server.set_command_line(argc, argv);
    server.set_configuration_option_default("cursor", "null");

    server.apply_settings();

    auto const options = server.get_options();
    EXPECT_THAT(options->get<std::string>("cursor"), StrEq("software"));
}

TEST_F(ServerConfigurationOptions, environment_takes_precedence_over_overridden_default)
{
    add_to_environment("MIR_SERVER_CURSOR", "software");
    server.set_configuration_option_default("cursor", "null");

    server.apply_settings();

    auto const options = server.get_options();
    EXPECT_THAT(options->get<std::string>("cursor"), StrEq("software"));
}

TEST_F(ServerConfigurationOptions, overridden_default_of_wrong_type_is_an_error)
{
    server.set_configuration_option_default("composite-delay", "42");

    EXPECT_THROW(server.apply_settings(), std::logic_error);
}

TEST_F(ServerConfigurationOptions, overridden_default_for_unknown_option_is_ignored)
{
    server.set_configuration_option_default("not-actually-an-option-defined", "a string");

    server.apply_settings();

    auto const options = server.get_options();
    EXPECT_FALSE(options->is_set("not-actually-an-option-defined"));
}

TEST_F(ServerConfigurationOptions, overridden_default_is_shown_in_help)
{
    const int argc = 2;
    char const* argv[argc] = {__PRETTY_FUNCTION__, "--help"};

    server.set_command_line(argc, argv);
    server.set_configuration_option_default("cursor", "null");

    try
    {
        server.apply_settings();
        server.get_options();
        FAIL() << "Expected help output";
    }
    catch (mir::ExitWithOutput const& output)
    {
        EXPECT_THAT(output.what(), HasSubstr("--cursor arg (=null)"));
    }
}

TEST_F(ServerConfigurationOptions, overridden_default_is_shown_in_help_markdown)
{
    const int argc = 2;
    char const* argv[argc] = {__PRETTY_FUNCTION__, "--help-markdown"};

    server.set_command_line(argc, argv);
    server.set_configuration_option_default("composite-delay", 42);

    try
    {
        server.apply_settings();
        server.get_options();
        FAIL() << "Expected help output";
    }
    catch (mir::ExitWithOutput const& output)
    {
        EXPECT_THAT(output.what(), HasSubstr("Defaults to `42`."));
    }
}
