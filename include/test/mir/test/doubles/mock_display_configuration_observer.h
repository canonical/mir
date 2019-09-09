/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com
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
    MOCK_METHOD1(initial_configuration, void(std::shared_ptr<graphics::DisplayConfiguration const> const& config));

    MOCK_METHOD1(configuration_applied, void(std::shared_ptr<graphics::DisplayConfiguration const> const& config));

    MOCK_METHOD1(
        base_configuration_updated,
        void(std::shared_ptr<graphics::DisplayConfiguration const> const& base_config));

    MOCK_METHOD2(
        session_configuration_applied,
        void(
            std::shared_ptr<scene::Session> const& session,
            std::shared_ptr<graphics::DisplayConfiguration> const& config));

    MOCK_METHOD1(session_configuration_removed, void(std::shared_ptr<scene::Session> const& session));

    MOCK_METHOD2(
        configuration_failed,
        void(std::shared_ptr<graphics::DisplayConfiguration const> const& attempted, std::exception const& error));

    MOCK_METHOD2(
        catastrophic_configuration_error,
        void(
            std::shared_ptr<graphics::DisplayConfiguration const> const& failed_fallback,
            std::exception const& error));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_CONFIGURATION_OBSERVER_H_ */

