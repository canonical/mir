/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Joseph Rushton Wakeling <joe [@t] webdrake.net>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/command_line_option.h"

#include <gtest/gtest.h>

using namespace miral;
using namespace testing;

TEST(CommandLineOption, can_take_lambdas)
{
    // ambiguous between all four possible 3-parameter overloads
    auto bool_flag = miral::CommandLineOption(
        [](bool /*is_set*/) {}, "bool", "Set a boolean flag");

    // ambiguous between `void(bool)`, `void(mir::optional_value<bool> const&)`
    // and `void(mir::optional_value<int> const&)`
    auto optional_bool_flag = miral::CommandLineOption(
        [](mir::optional_value<bool> const& /*value*/) {}, "optional-bool", "Set an optional boolean flag");

    // ambiguous between `void(bool)` vs. `void(mir::optional_value<int> const&)`
    auto optional_int_flag = miral::CommandLineOption(
        [](mir::optional_value<int> const& /*value*/) {}, "optional-int", "Set an optional int flag");

    // works
    auto optional_string_flag = miral::CommandLineOption(
        [](mir::optional_value<std::string> const& /*value*/) {}, "optional-string", "Set an optional string flag");
}
