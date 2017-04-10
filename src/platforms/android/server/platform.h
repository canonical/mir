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
#include "mir/graphics/display.h"
#include "mir/renderer/gl/egl_platform.h"

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
class CommandStreamSyncFactory;
class NativeWindowReport;


class GrallocPlatform : public graphics::RenderingPlatform,
                        public graphics::NativeRenderingPlatform,
                        public renderer::gl::EGLPlatform
{
public:
    GrallocPlatform(
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator);

    UniqueModulePtr<graphics::GraphicBufferAllocator> create_buffer_allocator() override;
    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;
    NativeRenderingPlatform* native_rendering_platform() override;
    EGLNativeDisplayType egl_native_display() const override;

private:
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;
};

class HwcPlatform : public graphics::DisplayPlatform
{
public:
    HwcPlatform(
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<DisplayComponentFactory> const& display_buffer_builder,
        std::shared_ptr<DisplayReport> const& display_report,
        std::shared_ptr<NativeWindowReport> const& native_window_report,
        OverlayOptimization overlay_option,
        std::shared_ptr<DeviceQuirks> const& quirks);

    UniqueModulePtr<Display> create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
        std::shared_ptr<graphics::GLConfig> const& /*gl_config*/) override;
    NativeDisplayPlatform* native_display_platform() override;
    std::vector<mir::ExtensionDescription> extensions() const override;

private:
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<DisplayComponentFactory> const display_buffer_builder;
    std::shared_ptr<DisplayReport> const display_report;
    std::shared_ptr<DeviceQuirks> const quirks;
    std::shared_ptr<NativeWindowReport> const native_window_report;
    OverlayOptimization const overlay_option;
};

class Platform : public graphics::Platform
{
public:
    Platform(
        std::shared_ptr<DisplayPlatform> const& display,
        std::shared_ptr<GrallocPlatform> const& rendering);

    UniqueModulePtr<graphics::GraphicBufferAllocator> create_buffer_allocator() override;
    UniqueModulePtr<Display> create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
        std::shared_ptr<graphics::GLConfig> const& /*gl_config*/) override;
    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;
    NativeRenderingPlatform* native_rendering_platform() override;
    NativeDisplayPlatform* native_display_platform() override;
    std::vector<mir::ExtensionDescription> extensions() const override;

private:
    std::shared_ptr<DisplayPlatform> const display;
    std::shared_ptr<GrallocPlatform> const rendering;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_PLATFORM_H_ */
