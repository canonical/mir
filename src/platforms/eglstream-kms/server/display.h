/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_PLATFORMS_EGLSTREAM_KMS_DISPLAY_H_
#define MIR_PLATFORMS_EGLSTREAM_KMS_DISPLAY_H_

#include "eglstream_display_target.h"

#include "mir/graphics/display.h"
#include "kms_display_configuration.h"
#include "mir/fd.h"
#include "mir/graphics/platform.h"

#include <mutex>

namespace mir
{
namespace graphics
{
class DisplayConfigurationPolicy;
class GLConfig;
class DisplayReport;

namespace eglstream
{
class DRMEventHandler;

class Display : public mir::graphics::Display
{
public:
    Display(
        std::shared_ptr<EGLStreamDisplayTarget> provider,
        mir::Fd drm_node,
        EGLDisplay display,
        std::shared_ptr<DisplayConfigurationPolicy> const& configuration_policy,
        GLConfig const& gl_conf,
        std::shared_ptr<DisplayReport> display_report);

    void for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f) override;

    std::unique_ptr<DisplayConfiguration> configuration() const override;

    bool apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) override;

    void configure(DisplayConfiguration const& conf) override;

    void register_configuration_change_handler(EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void pause() override;

    void resume() override;

    std::shared_ptr<Cursor> create_hardware_cursor() override;

private:
    std::shared_ptr<EGLStreamDisplayTarget> const provider;
    mir::Fd const drm_node;
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;

    std::mutex mutable configuration_mutex;
    KMSDisplayConfiguration display_configuration;

    std::shared_ptr<DRMEventHandler> const event_handler;
    std::vector<std::unique_ptr<DisplaySyncGroup>> active_sync_groups;
    std::shared_ptr<DisplayConfigurationPolicy> const configuration_policy;
    std::shared_ptr<DisplayReport> const display_report;
};

}
}

}

#endif  // MIR_PLATFORMS_EGLSTREAM_KMS_DISPLAY_H_
