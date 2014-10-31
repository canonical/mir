/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/headless_test.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

struct ServerCommandLine : mir_test_framework::HeadlessTest
{
    MOCK_METHOD1(command_line_handler, void(std::vector<std::string> const&));

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
    }
};

TEST_F(ServerCommandLine, unknown_options_are_passed_to_handler)
{
    const int argc = 10;
    char const* argv[argc] = {
        __PRETTY_FUNCTION__,
        "--enable-input", "no",
        "--hello",
        "-f", "test_file",
        "world",
        "--offscreen",
        "--answer", "42"
    };

    server.set_command_line(argc, argv);

    EXPECT_CALL(*this, command_line_handler(
        ElementsAre(StrEq("--hello"), StrEq("world"), StrEq("--answer"), StrEq("42"))));

    server.the_session_authorizer();
}
