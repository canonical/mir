/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORMS_EGLSTREAM_KMS_DISPLAY_H_
#define MIR_PLATFORMS_EGLSTREAM_KMS_DISPLAY_H_

#include "mir/graphics/display.h"
#include "kms_display_configuration.h"
#include "mir/fd.h"
#include "mir/renderer/gl/context_source.h"

namespace mir
{
namespace graphics
{
class DisplayConfigurationPolicy;
class GLConfig;

namespace eglstream
{

class Display : public mir::graphics::Display,
                public mir::graphics::NativeDisplay,
                public mir::renderer::gl::ContextSource
{
public:
    Display(
        mir::Fd drm_node,
        EGLDisplay display,
        std::shared_ptr<DisplayConfigurationPolicy> const& configuration_policy,
        GLConfig const& gl_conf);

    void for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f) override;

    std::unique_ptr<DisplayConfiguration> configuration() const override;

    bool apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) override;

    void configure(DisplayConfiguration const& conf) override;

    void register_configuration_change_handler(EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void register_pause_resume_handlers(EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler, DisplayResumeHandler const& resume_handler) override;

    void pause() override;

    void resume() override;

    std::shared_ptr<Cursor> create_hardware_cursor() override;

    std::unique_ptr<VirtualOutput> create_virtual_output(int width, int height) override;

    NativeDisplay* native_display() override;

    std::unique_ptr<renderer::gl::Context> create_gl_context() const override;
    Frame last_frame_on(unsigned output_id) const override;

private:
    mir::Fd const drm_node;
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    KMSDisplayConfiguration display_configuration;
    std::vector<std::unique_ptr<DisplaySyncGroup>> active_sync_groups;
    std::shared_ptr<DisplayConfigurationPolicy> const configuration_policy;
};

}
}

}

#endif  // MIR_PLATFORMS_EGLSTREAM_KMS_DISPLAY_H_
