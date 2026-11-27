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

#include "performance_test_utils.h"

#include <gtest/gtest.h>

#include <fstream>
#include <string_view>

void mir::test::record_properties(std::string const& log_filename, std::string const& prefix)
{
    constexpr std::string_view renderer_marker{"GL renderer:"};
    constexpr std::string_view mode_marker{"Current mode"};

    std::ifstream logs{log_filename};
    ASSERT_TRUE(logs) << "Failed to open log file for analysis: " << log_filename;

    std::string renderer;
    std::string mode;
    std::string line;
    while (std::getline(logs, line))
    {
        if (auto const n = line.find(renderer_marker); n != std::string::npos)
        {
            renderer = line.substr(n + renderer_marker.size());
        }
        else if (auto const n = line.find(mode_marker); n != std::string::npos)
        {
            mode = line.substr(n + mode_marker.size());
            break;
        }
    }
    testing::Test::RecordProperty(prefix + "_renderer", renderer);
    testing::Test::RecordProperty(prefix + "_mode", mode);
}
