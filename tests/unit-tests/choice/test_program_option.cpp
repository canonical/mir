/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/choice/program_option.h"

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/parsers.hpp>

#include <cstdlib>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace bpo = boost::program_options;

namespace
{
struct ProgramOption : testing::Test
{
    ProgramOption() :
        desc("Options")
    {
        desc.add_options()
            ("file,f", bpo::value<std::string>(), "<socket filename>")
            ("help,h", "this help text");
    }

    bpo::options_description desc;
};
}

TEST_F(ProgramOption, parse_command_line_long)
{
    mir::choice::ProgramOption po;

    const int argc = 3;
    char const* argv[argc] = {
        __PRETTY_FUNCTION__,
        "--file", "test_file"
    };

    po.parse_arguments(desc, argc, argv);

    EXPECT_EQ("test_file", po.get("file", "default"));
    EXPECT_EQ("default", po.get("garbage", "default"));
    EXPECT_TRUE(po.is_set("file"));
    EXPECT_FALSE(po.is_set("garbage"));
}

TEST_F(ProgramOption, parse_command_line_short)
{
    mir::choice::ProgramOption po;

    const int argc = 3;
    char const* argv[argc] = {
        __PRETTY_FUNCTION__,
        "-f", "test_file"
    };

    po.parse_arguments(desc, argc, argv);

    EXPECT_EQ("test_file", po.get("file", "default"));
    EXPECT_EQ("default", po.get("garbage", "default"));
    EXPECT_TRUE(po.is_set("file"));
    EXPECT_FALSE(po.is_set("garbage"));
}

TEST_F(ProgramOption, parse_command_line_help)
{
    mir::choice::ProgramOption po;

    const int argc = 2;
    char const* argv[argc] = {
        __PRETTY_FUNCTION__,
        "--help"
    };

    po.parse_arguments(desc, argc, argv);

    EXPECT_TRUE(po.is_set("help"));
}

TEST_F(ProgramOption, parse_environment)
{
    mir::choice::ProgramOption po;

    std::string const prefix(__PRETTY_FUNCTION__);
    char const* key = "some_key";
    char const* value = "test_value";

    std::string const env = prefix + key;

    setenv(env.c_str(), value, true);

    po.parse_environment(desc, __PRETTY_FUNCTION__);

    EXPECT_EQ(value, po.get(key, "default"));
    EXPECT_EQ("default", po.get("garbage", "default"));
    EXPECT_TRUE(po.is_set(key));
    EXPECT_FALSE(po.is_set("garbage"));
}

