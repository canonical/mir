/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_CONFIG_CHANGER_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_CONFIG_CHANGER_H_

#include "mir/frontend/input_configuration_changer.h"
#include "mir/input/input_configuration.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
class MockInputConfigurationChanger : public frontend::InputConfigurationChanger
{
public:
    MOCK_METHOD0(base_configuration, input::InputConfiguration());
    MOCK_METHOD2(configure_called, void(std::shared_ptr<frontend::Session> const&, input::InputConfiguration const&));
    MOCK_METHOD1(set_base_configuration_called, void(input::InputConfiguration const&));

    void configure(std::shared_ptr<frontend::Session> const& session, input::InputConfiguration && conf) override
    {
        configure_called(session, conf);
    }
    void set_base_configuration(input::InputConfiguration && conf) override
    {
        set_base_configuration_called(conf);
    }
};
}
}
}

#endif

