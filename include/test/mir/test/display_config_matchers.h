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

#ifndef MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_
#define MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_

#include <mir_toolkit/client_types.h>

#include <memory>
#include <gmock/gmock.h>



namespace mir
{

namespace graphics
{
class DisplayConfiguration;
}

namespace test
{

bool compare_display_configurations(
    testing::MatchResultListener* listener,
    graphics::DisplayConfiguration const& display_config1,
    graphics::DisplayConfiguration const& display_config2);

bool compare_display_configurations(
    testing::MatchResultListener* listener,
    std::shared_ptr<graphics::DisplayConfiguration const> const& display_config1,
    graphics::DisplayConfiguration const& display_config2);

MATCHER_P(DisplayConfigMatches, config, "")
{
    return compare_display_configurations(result_listener, arg, config);
}

}
}

#endif /* MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_ */
