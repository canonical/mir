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
#include <variant>

namespace mir
{
class EmergencyCleanupRegistry;
class ConsoleServices;

namespace graphics
{
namespace gbm
{

class Quirks;

class Platform : public graphics::DisplayPlatform
{
public:
    Platform(
        udev::Device const& device,
        std::shared_ptr<DisplayReport> const& reporter,
        ConsoleServices& vt,
        EmergencyCleanupRegistry& emergency_cleanup_registry,
        BypassOption bypass_option);

    /* From Platform */
    UniqueModulePtr<graphics::Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config,
        std::shared_ptr<mir::options::Option> const& options) override;

    std::shared_ptr<mir::udev::Context> udev;
    
    std::shared_ptr<DisplayReport> const listener;

protected:
    auto interface_for() -> std::shared_ptr<DisplayInterfaceProvider> override;

public:
    BypassOption bypass_option() const;
private:
    Platform(
        std::tuple<std::unique_ptr<Device>, mir::Fd> drm,
        std::shared_ptr<DisplayReport> const& reporter,
        EmergencyCleanupRegistry& emergency_cleanup_registry,
        BypassOption bypass_option);
    
    std::unique_ptr<Device> const device_handle;
    mir::Fd const drm_fd;

    class KMSDisplayInterfaceProvider;
    std::shared_ptr<KMSDisplayInterfaceProvider> const provider;
    
    BypassOption const bypass_option_;
};

class RenderingPlatform : public graphics::RenderingPlatform
{
public:
    RenderingPlatform(
        udev::Device const& device,
        std::vector<std::shared_ptr<graphics::DisplayInterfaceProvider>> const& displays);

    auto create_buffer_allocator(
        graphics::Display const&) -> UniqueModulePtr<graphics::GraphicBufferAllocator> override;

protected:
    auto maybe_create_interface(
        RendererInterfaceBase::Tag const& type_tag) -> std::shared_ptr<RendererInterfaceBase> override;

private:
    RenderingPlatform(
        std::variant<std::shared_ptr<GBMDisplayProvider>, std::shared_ptr<gbm_device>> hw);
    
    std::shared_ptr<gbm_device> const device;                   ///< gbm_device this platform is created on, always valid.
    std::shared_ptr<GBMDisplayProvider> const bound_display;    ///< Associated Display, if any (nullptr is valid)
    EGLDisplay const dpy;
    EGLContext const share_ctx;
};
}
}
}
#endif /* MIR_GRAPHICS_GBM_PLATFORM_H_ */
