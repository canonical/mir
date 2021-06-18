/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/display_config_matchers.h"
#include "mir/graphics/display_configuration.h"
#include "mir_toolkit/client_types.h"
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    mg::DisplayConfiguration const& config1,
    mg::DisplayConfiguration const& config2)
{
    using namespace testing;
    bool failure = false;

    /* Outputs */
    std::vector<mg::DisplayConfigurationOutput> outputs1;
    std::vector<mg::DisplayConfigurationOutput> outputs2;

    config1.for_each_output(
        [&outputs1](mg::DisplayConfigurationOutput const& output)
        {
            outputs1.push_back(output);
        });
    config2.for_each_output(
        [&outputs2](mg::DisplayConfigurationOutput const& output)
        {
            outputs2.push_back(output);
        });

    failure |= !ExplainMatchResult(UnorderedElementsAreArray(outputs1), outputs2, listener);

    return !failure;
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    std::shared_ptr<mg::DisplayConfiguration const> const& display_config1,
    mg::DisplayConfiguration const& display_config2)
{
    return compare_display_configurations(listener, *display_config1, display_config2);
}
