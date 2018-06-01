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

#ifndef MIR_PLATFORMS_EGLSTREAM_KMS_PLATFORM_H_
#define MIR_PLATFORMS_EGLSTREAM_KMS_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/fd.h"
#include "mir/renderer/gl/egl_platform.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace mir
{
class Device;

namespace graphics
{
namespace eglstream
{

class RenderingPlatform : public graphics::RenderingPlatform
{
public:
    UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator() override;
    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;
    NativeRenderingPlatform* native_rendering_platform() override;
};

class DisplayPlatform : public graphics::DisplayPlatform
{
public:
    DisplayPlatform(ConsoleServices& console, EGLDeviceEXT device);

    UniqueModulePtr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
        std::shared_ptr<GLConfig> const& /*gl_config*/) override;
    NativeDisplayPlatform* native_display_platform() override;
    std::vector<ExtensionDescription> extensions() const override;

private:
    EGLDisplay display;
    mir::Fd drm_node;
    std::unique_ptr<mir::Device> drm_device;
};

class Platform : public graphics::Platform
{
public:
    Platform(
        std::shared_ptr<RenderingPlatform> const&,
        std::shared_ptr<DisplayPlatform> const&);
    ~Platform() = default;

    UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator() override;

    UniqueModulePtr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
        std::shared_ptr<GLConfig> const& /*gl_config*/) override;
    NativeDisplayPlatform* native_display_platform() override;
    std::vector<ExtensionDescription> extensions() const override;

    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;

    NativeRenderingPlatform* native_rendering_platform() override;

private:
    std::shared_ptr<RenderingPlatform> const rendering;
    std::shared_ptr<DisplayPlatform> const display;
};
}
}
}

#endif // MIR_PLATFORMS_EGLSTREAM_KMS_PLATFORM_H_
