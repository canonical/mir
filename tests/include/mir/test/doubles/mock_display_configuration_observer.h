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

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_CONFIGURATION_OBSERVER_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_CONFIGURATION_OBSERVER_H_

#include "mir/graphics/display_configuration_observer.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockDisplayConfigurationObserver : public graphics::DisplayConfigurationObserver
{
public:
    MOCK_METHOD(void, initial_configuration,
                (std::shared_ptr<graphics::DisplayConfiguration const> const& config), (override));

    MOCK_METHOD(void, configuration_applied,
                (std::shared_ptr<graphics::DisplayConfiguration const> const& config), (override));

    MOCK_METHOD(void, base_configuration_updated,
        (std::shared_ptr<graphics::DisplayConfiguration const> const& base_config), (override));

    MOCK_METHOD(void, session_configuration_applied,
        (std::shared_ptr<scene::Session> const& session, std::shared_ptr<graphics::DisplayConfiguration> const& config),
        (override));

    MOCK_METHOD(void, session_configuration_removed, (std::shared_ptr<scene::Session> const& session), (override));

    MOCK_METHOD(void, configuration_failed,
        (std::shared_ptr<graphics::DisplayConfiguration const> const& attempted, std::exception const& error),
        (override));

    MOCK_METHOD(void, catastrophic_configuration_error,
        (std::shared_ptr<graphics::DisplayConfiguration const> const& failed_fallback, std::exception const& error),
        (override));

    MOCK_METHOD(void, configuration_updated_for_session,
        (std::shared_ptr<scene::Session> const&, std::shared_ptr<graphics::DisplayConfiguration const> const&),
        (override));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_CONFIGURATION_OBSERVER_H_ */

