/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/gl_context.h"
#include "mir/main_loop.h"
#include "mir/test/gmock_fixes.h"
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
    MOCK_METHOD1(for_each_display_sync_group, void (std::function<void(graphics::DisplaySyncGroup&)> const&));
    MOCK_CONST_METHOD0(configuration, std::unique_ptr<graphics::DisplayConfiguration>());
    MOCK_METHOD1(configure, void(graphics::DisplayConfiguration const&));
    MOCK_METHOD2(register_configuration_change_handler,
                 void(graphics::EventHandlerRegister&, graphics::DisplayConfigurationChangeHandler const&));

    MOCK_METHOD3(register_pause_resume_handlers, void(graphics::EventHandlerRegister&,
                                                      graphics::DisplayPauseHandler const&,
                                                      graphics::DisplayResumeHandler const&));
    MOCK_METHOD0(pause, void());
    MOCK_METHOD0(resume, void());
    MOCK_METHOD1(create_hardware_cursor, std::shared_ptr<graphics::Cursor>(std::shared_ptr<graphics::CursorImage> const&));
    MOCK_METHOD0(create_gl_context, std::unique_ptr<graphics::GLContext>());
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_H_ */
