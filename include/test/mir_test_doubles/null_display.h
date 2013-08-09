/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_DISPLAY_H_
#define MIR_TEST_DOUBLES_NULL_DISPLAY_H_

#include "mir/graphics/display.h"
#include "null_gl_context.h"
#include "null_display_configuration.h"
#include <thread>

namespace mir
{
namespace test
{
namespace doubles
{

class NullDisplay : public graphics::Display
{
 public:
    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const&)
    {
        /* yield() is needed to ensure reasonable runtime under valgrind for some tests */
        std::this_thread::yield();
    }
    std::shared_ptr<graphics::DisplayConfiguration> configuration()
    {
        return std::make_shared<NullDisplayConfiguration>();
    }
    void configure(graphics::DisplayConfiguration const&) {}
    void register_configuration_change_handler(
        graphics::EventHandlerRegister&,
        graphics::DisplayConfigurationChangeHandler const&) override
    {
    }
    void register_pause_resume_handlers(graphics::EventHandlerRegister&,
                                        graphics::DisplayPauseHandler const&,
                                        graphics::DisplayResumeHandler const&) override
    {
    }
    void pause() {}
    void resume() {}
    std::weak_ptr<graphics::Cursor> the_cursor() { return {}; }
    std::unique_ptr<graphics::GLContext> create_gl_context()
    {
        return std::unique_ptr<NullGLContext>{new NullGLContext()};
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_DISPLAY_H_ */
