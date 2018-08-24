/*
 * Copyright © 2015 Canonical Ltd.
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
    MOCK_CONST_METHOD1(
        for_each_card,
        void(std::function<void(graphics::DisplayConfigurationCard const&)>));

    MOCK_CONST_METHOD1(
        for_each_output,
        void(std::function<void(graphics::DisplayConfigurationOutput const&)>));

    MOCK_METHOD1(
        for_each_output,
        void(std::function<void(graphics::UserDisplayConfigurationOutput&)>));

    MOCK_CONST_METHOD0(valid, bool());

    std::unique_ptr<graphics::DisplayConfiguration> clone() const
    {
        throw std::runtime_error("MockDisplayConfiguration::clone is not implemented");
    }
};

}
}
}

#endif
