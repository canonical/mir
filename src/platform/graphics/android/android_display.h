/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_H_

#include "mir/graphics/display.h"
#include "gl_context.h"

#include <memory>

namespace mir
{
namespace graphics
{

class DisplayReport;

namespace android
{
class DisplayBuilder;
class DisplaySupportProvider;
class ConfigurableDisplayBuffer;

class AndroidDisplay : public Display
{
public:
    explicit AndroidDisplay(std::shared_ptr<DisplayBuilder> const& display_builder,
                            std::shared_ptr<DisplayReport> const& display_report);

    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const& f);

    std::unique_ptr<DisplayConfiguration> configuration() const override;
    void configure(DisplayConfiguration const&) override;

    void register_configuration_change_handler(
        EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler);

    void register_pause_resume_handlers(
        EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler);

    void pause();
    void resume();

    std::weak_ptr<Cursor> the_cursor();
    std::unique_ptr<graphics::GLContext> create_gl_context();

private:
    std::shared_ptr<DisplayBuilder> const display_builder;
    GLContext gl_context;

    //we only have a primary display at the moment
    std::unique_ptr<ConfigurableDisplayBuffer> const display_buffer;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_H_ */
