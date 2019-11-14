/*
 * Copyright © 2019 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_WAYLAND_PLATFORM_H_
#define MIR_GRAPHICS_WAYLAND_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/fd.h"
#include "mir/renderer/gl/egl_platform.h"

#include "displayclient.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client.h>
#include <wayland-egl.h>

namespace mir
{
namespace graphics
{
namespace wayland
{
class Platform : public graphics::Platform,
                 public graphics::NativeRenderingPlatform,
                 public mir::renderer::gl::EGLPlatform
{
public:
    Platform(struct wl_display* const wl_display, std::shared_ptr<DisplayReport> const& report);
    ~Platform() = default;

    UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator(Display const& output) override;

    UniqueModulePtr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config) override;
    NativeDisplayPlatform* native_display_platform() override;
    std::vector<ExtensionDescription> extensions() const override;

    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;

    NativeRenderingPlatform* native_rendering_platform() override;
    EGLNativeDisplayType egl_native_display() const override;

private:
    struct wl_display* const wl_display;
    std::shared_ptr<DisplayReport> const report;
};
}
}
}

#endif // MIR_GRAPHICS_WAYLAND_PLATFORM_H_
