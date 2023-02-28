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
    explicit Platform(std::shared_ptr<DisplayReport> const& reporter,
                      ConsoleServices& vt,
                      EmergencyCleanupRegistry& emergency_cleanup_registry,
                      BypassOption bypass_option,
                      std::unique_ptr<Quirks> quirks);

    /* From Platform */
    UniqueModulePtr<graphics::Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config) override;

    std::shared_ptr<mir::udev::Context> udev;
    std::vector<std::shared_ptr<helpers::DRMHelper>> const drm;
    std::shared_ptr<helpers::GBMHelper> const gbm;

    std::shared_ptr<DisplayReport> const listener;

    BypassOption bypass_option() const;
private:
    BypassOption const bypass_option_;
};

class RenderingPlatform : public graphics::RenderingPlatform
{
public:
    RenderingPlatform(udev::Device const& device, std::vector<std::shared_ptr<graphics::DisplayPlatform>> const& displays);

    auto create_buffer_allocator(
        graphics::Display const& output) -> UniqueModulePtr<graphics::GraphicBufferAllocator> override;
};

}
}
}
#endif /* MIR_GRAPHICS_GBM_PLATFORM_H_ */
