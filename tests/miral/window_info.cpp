/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <miral/window_info.h>

#include "mir_test_framework/process.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace miral;
namespace mtf = mir_test_framework;

namespace
{
static int a_successful_exit_function()
{
    return EXIT_SUCCESS;
}
}

// Test for crash http://paste.ubuntu.com/25523431/
TEST(WindowInfo, negative_window_size_does_not_divide_by_zero)
{
    auto p = mtf::fork_and_run_in_a_different_process(
        [] {
            Window window;
            WindowSpecification params;

            Point p{0, 0};
            Size  s{-300, -300};

            params.name()     = "";
            params.top_left() = p;
            params.size()     = s;

            WindowInfo info(window, params);

            info.min_width(s.width);
            info.min_height(s.height);

            info.constrain_resize(p, s);
        }, a_successful_exit_function);

    EXPECT_TRUE(p->wait_for_termination().succeeded());
}
