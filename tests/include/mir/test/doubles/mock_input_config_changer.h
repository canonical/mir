/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "mir/input/mir_input_config.h"

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
    MOCK_METHOD0(base_configuration, MirInputConfig());
    MOCK_METHOD2(configure_called, void(std::shared_ptr<scene::Session> const&, MirInputConfig const&));
    MOCK_METHOD1(set_base_configuration_called, void(MirInputConfig const&));

    void configure(std::shared_ptr<scene::Session> const& session, MirInputConfig && conf) override
    {
        configure_called(session, conf);
    }
    void set_base_configuration(MirInputConfig && conf) override
    {
        set_base_configuration_called(conf);
    }
};
}
}
}

#endif

