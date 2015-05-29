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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_PLATFORM_H_
#define MIR_GRAPHICS_MESA_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "platform_common.h"
#include "display_helpers.h"

namespace mir
{
class EmergencyCleanupRegistry;
namespace graphics
{
namespace mesa
{

class VirtualTerminal;
class InternalNativeDisplay;
class Platform : public graphics::Platform,
                 public std::enable_shared_from_this<Platform>
{
public:
    explicit Platform(std::shared_ptr<DisplayReport> const& reporter,
                      std::shared_ptr<VirtualTerminal> const& vt,
                      EmergencyCleanupRegistry& emergency_cleanup_registry,
                      BypassOption bypass_option);

    /* From Platform */
    std::shared_ptr<graphics::GraphicBufferAllocator> create_buffer_allocator() override;
    std::shared_ptr<graphics::Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLProgramFactory> const& program_factory,
        std::shared_ptr<GLConfig> const& gl_config) override;

    std::shared_ptr<PlatformIpcOperations> make_ipc_operations() const override;

    EGLNativeDisplayType egl_native_display() const override;

    std::shared_ptr<mir::udev::Context> udev;
    std::shared_ptr<helpers::DRMHelper> const drm;
    helpers::GBMHelper gbm;

    std::shared_ptr<DisplayReport> const listener;
    std::shared_ptr<VirtualTerminal> const vt;

    BypassOption bypass_option() const;
private:
    BypassOption const bypass_option_;
};

}
}
}
#endif /* MIR_GRAPHICS_MESA_PLATFORM_H_ */
