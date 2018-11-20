/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_NULL_DISPLAY_H_
#define MIR_TEST_DOUBLES_NULL_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/virtual_output.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/test/doubles/null_gl_context.h"
#include "mir/test/doubles/null_display_configuration.h"
#include "mir/test/doubles/null_display_sync_group.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullDisplay : public graphics::Display,
                    public graphics::NativeDisplay,
                    public renderer::gl::ContextSource
{
 public:
    void for_each_display_sync_group(std::function<void(graphics::DisplaySyncGroup&)> const& f) override
    {
        f(group);
    }
    std::unique_ptr<graphics::DisplayConfiguration> configuration() const override
    {
        return std::unique_ptr<graphics::DisplayConfiguration>(
            new NullDisplayConfiguration
        );
    }
    bool apply_if_configuration_preserves_display_buffers(graphics::DisplayConfiguration const&) override
    {
        return false;
    }
    void configure(graphics::DisplayConfiguration const&)  override{}
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
    void pause() override{}
    void resume() override {}

    std::shared_ptr<graphics::Cursor> create_hardware_cursor() override
    {
         return {}; 
    }
    std::unique_ptr<graphics::VirtualOutput> create_virtual_output(int /*width*/, int /*height*/) override
    {
        return nullptr;
    }
    graphics::NativeDisplay* native_display() override
    {
        return this;
    }
    std::unique_ptr<renderer::gl::Context> create_gl_context() const override
    {
        return std::unique_ptr<NullGLContext>{new NullGLContext()};
    }
    graphics::Frame last_frame_on(unsigned) const override
    {
        return {};
    }
    NullDisplaySyncGroup group;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_DISPLAY_H_ */
