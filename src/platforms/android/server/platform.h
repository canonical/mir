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

#ifndef MIR_GRAPHICS_ANDROID_PLATFORM_H_
#define MIR_GRAPHICS_ANDROID_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "device_quirks.h"
#include "overlay_optimization.h"

namespace mir
{
namespace graphics
{
class DisplayReport;
namespace android
{
class GraphicBufferAllocator;
class FramebufferFactory;
class DisplayComponentFactory;

class Platform : public graphics::Platform
{
public:
    Platform(
        std::shared_ptr<DisplayComponentFactory> const& display_buffer_builder,
        std::shared_ptr<DisplayReport> const& display_report,
        OverlayOptimization overlay_option,
        std::shared_ptr<DeviceQuirks> const& quirks);

    /* From Platform */
    std::shared_ptr<graphics::GraphicBufferAllocator> create_buffer_allocator() override;
    std::shared_ptr<Display> create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
        std::shared_ptr<graphics::GLProgramFactory> const&,
        std::shared_ptr<graphics::GLConfig> const& /*gl_config*/) override;
    std::shared_ptr<PlatformIpcOperations> make_ipc_operations() const override;
    EGLNativeDisplayType egl_native_display() const override;

private:
    std::shared_ptr<GraphicBufferAllocator> create_mga_buffer_allocator();

    std::shared_ptr<DisplayComponentFactory> const display_buffer_builder;
    std::shared_ptr<DisplayReport> const display_report;
    std::shared_ptr<PlatformIpcOperations> const ipc_operations;
    std::shared_ptr<DeviceQuirks> const quirks;
    OverlayOptimization const overlay_option;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_PLATFORM_H_ */
