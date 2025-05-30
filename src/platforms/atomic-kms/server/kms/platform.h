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

#ifndef MIR_GRAPHICS_GBM_PLATFORM_H_
#define MIR_GRAPHICS_GBM_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "platform_common.h"
#include "display_helpers.h"

#include <EGL/egl.h>

namespace mir
{
class EmergencyCleanupRegistry;
class ConsoleServices;

namespace graphics
{
namespace atomic
{

class RuntimeQuirks;

class Platform : public graphics::DisplayPlatform
{
public:
    Platform(
        udev::Device const& device,
        std::shared_ptr<DisplayReport> const& reporter,
        ConsoleServices& vt,
        EmergencyCleanupRegistry& emergency_cleanup_registry,
        BypassOption bypass_option,
        std::shared_ptr<RuntimeQuirks> runtime_quirks);

    ~Platform() override;

    /* From Platform */
    UniqueModulePtr<graphics::Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config) override;

    std::shared_ptr<mir::udev::Context> udev;
    
    std::shared_ptr<DisplayReport> const listener;

protected:
    auto maybe_create_provider(DisplayProvider::Tag const& type_tag)
        -> std::shared_ptr<DisplayProvider> override;

public:
    BypassOption bypass_option() const;
private:
    Platform(
        std::tuple<std::unique_ptr<Device>, mir::Fd> drm,
        std::shared_ptr<DisplayReport> const& reporter,
        EmergencyCleanupRegistry& emergency_cleanup_registry,
        BypassOption bypass_option,
        std::shared_ptr<RuntimeQuirks> runtime_quirks);

    std::unique_ptr<Device> const device_handle;
    mir::Fd const drm_fd;

    std::shared_ptr<GBMDisplayProvider> gbm_display_provider;

    BypassOption const bypass_option_;
    std::shared_ptr<RuntimeQuirks> const runtime_quirks;
};
}
}
}
#endif /* MIR_GRAPHICS_GBM_PLATFORM_H_ */
