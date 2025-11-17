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

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_CONFIGURATION_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockDisplayConfiguration : public graphics::DisplayConfiguration
{
    MOCK_METHOD(void, for_each_output, (std::function<void(graphics::DisplayConfigurationOutput const&)>), (const));

    MOCK_METHOD(void, for_each_output, (std::function<void(graphics::UserDisplayConfigurationOutput&)>), (override));

    MOCK_METHOD(bool, valid, (), (const));

    std::unique_ptr<graphics::DisplayConfiguration> clone() const
    {
        throw std::runtime_error("MockDisplayConfiguration::clone is not implemented");
    }
};

}
}
}

#endif
