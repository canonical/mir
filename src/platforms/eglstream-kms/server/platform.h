/*
 * Copyright © 2016 Canonical Ltd.
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

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace mir
{
namespace graphics
{
namespace eglstream
{
class Platform : public mir::graphics::Platform
{
public:
    Platform(
        EGLDeviceEXT device,
        std::shared_ptr<EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
        std::shared_ptr<DisplayReport> const& /*report*/);
    ~Platform() = default;

    UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator() override;

    UniqueModulePtr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
        std::shared_ptr<GLConfig> const& /*gl_config*/) override;

    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;

    EGLNativeDisplayType egl_native_display() const override;

private:
    EGLDeviceEXT device;
    EGLDisplay display;
    mir::Fd const drm_node;
};
}
}
}

#endif // MIR_PLATFORMS_EGLSTREAM_KMS_PLATFORM_H_
