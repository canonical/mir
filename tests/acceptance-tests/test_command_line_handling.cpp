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

#include "mir/run_mir.h"
#include "mir/default_server_configuration.h"
#include "mir/options/default_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

namespace mo = mir::options;

struct CommandLineHandling : Test
{
    MOCK_METHOD1(callback, void(std::vector<char const*> const&));

    std::function<void(int argc, char const* const* argv)> callback_functor =
        [this](int argc, char const* const* argv)
        {
            // Copy to a vector as ElementsAre() is convenient for checking
            std::vector<char const*> const copy{argv, argv+argc};
            callback(copy);
        };

    void invoke_parsing(mo::Configuration& config) { config.the_options(); }

};

TEST_F(CommandLineHandling, valid_options_are_accepted_by_default_callback)
{
    char const* argv[] =
     { "dummy-exe-name", "--file", "test", "--enable-input", "off"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    mo::DefaultConfiguration config{argc, argv};

    invoke_parsing(config);
}

TEST_F(CommandLineHandling, unrecognised_tokens_cause_default_callback_to_throw)
{
    char const* argv[] =
     { "dummy-exe-name", "--file", "test", "--hello", "world", "--enable-input", "off"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    mo::DefaultConfiguration config{argc, argv};

    EXPECT_THROW(invoke_parsing(config), std::runtime_error);
}

TEST_F(CommandLineHandling, valid_options_are_not_passed_to_callback)
{
    char const* argv[] =
     { "dummy-exe-name", "--file", "test", "--enable-input", "off"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    EXPECT_CALL(*this, callback(ElementsAre()));

    mo::DefaultConfiguration config{argc, argv, callback_functor};

    invoke_parsing(config);
}

TEST_F(CommandLineHandling, unrecognised_tokens_are_passed_to_callback)
{
    char const* argv[] =
     { "dummy-exe-name", "--file", "test", "--hello", "world", "--enable-input", "off"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    EXPECT_CALL(*this, callback(ElementsAre(StrEq("--hello"), StrEq("world"))));

    mo::DefaultConfiguration config{argc, argv, callback_functor};

    invoke_parsing(config);
}
