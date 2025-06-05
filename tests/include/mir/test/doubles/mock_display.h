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

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/renderer/gl/context.h"
#include "mir/main_loop.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockDisplay : public graphics::Display
{
public:
    MOCK_METHOD(void, for_each_display_sync_group, (std::function<void(graphics::DisplaySyncGroup&)> const&), (override));
    MOCK_METHOD(std::unique_ptr<graphics::DisplayConfiguration>, configuration, (), (const override));
    MOCK_METHOD(bool, apply_if_configuration_preserves_display_buffers, (graphics::DisplayConfiguration const&), (override));
    MOCK_METHOD(void, configure, (graphics::DisplayConfiguration const&), (override));
    MOCK_METHOD(void, register_configuration_change_handler, (graphics::EventHandlerRegister&, graphics::DisplayConfigurationChangeHandler const&), (override));
    MOCK_METHOD(void, pause, (), (override));
    MOCK_METHOD(void, resume, (), (override));
    MOCK_METHOD(std::shared_ptr<graphics::Cursor>, create_hardware_cursor, (), (override));
    MOCK_METHOD(graphics::Frame, last_frame_on, (unsigned), (const override));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_H_ */
