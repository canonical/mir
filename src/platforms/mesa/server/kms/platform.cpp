/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "platform.h"
#include "buffer_allocator.h"
#include "display.h"
#include "linux_virtual_terminal.h"
#include "ipc_operations.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/platform_authentication.h"
#include "mir/graphics/native_buffer.h"
#include "mir/graphics/platform_authentication.h"
#include "mir/emergency_cleanup_registry.h"
#include "mir/udev/wrapper.h"
#include "mesa_extensions.h"
#include "mir/renderer/gl/texture_target.h"
#include "mir/graphics/buffer_basic.h"
#include "mir/graphics/egl_error.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

#define MIR_LOG_COMPONENT "platform-graphics-mesa"
#include "mir/log.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <mir/renderer/gl/texture_source.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mgmh = mgm::helpers;

mgm::Platform::Platform(std::shared_ptr<DisplayReport> const& listener,
                        std::shared_ptr<VirtualTerminal> const& vt,
                        EmergencyCleanupRegistry& emergency_cleanup_registry,
                        BypassOption bypass_option)
    : udev{std::make_shared<mir::udev::Context>()},
      drm{helpers::DRMHelper::open_all_devices(udev)},
      gbm{std::make_shared<mgmh::GBMHelper>()},
      listener{listener},
      vt{vt},
      bypass_option_{bypass_option}
{
    // We assume the first DRM device is the boot GPU, and arbitrarily pick it as our
    // shell renderer.
    //
    // TODO: expose multiple rendering GPUs to the shell.
    gbm->setup(*drm.front());

    std::weak_ptr<VirtualTerminal> weak_vt = vt;
    std::vector<std::weak_ptr<mgmh::DRMHelper>> weak_drm;

    for (auto const &helper : drm)
    {
        weak_drm.push_back(helper);
    }
    emergency_cleanup_registry.add(
        make_module_ptr<EmergencyCleanupHandler>(
            [weak_vt,weak_drm]
            {
                if (auto const vt = weak_vt.lock())
                    try { vt->restore(); } catch (...) {}

                for (auto helper : weak_drm)
                {
                    if (auto const drm = helper.lock())
                    {
                        try
                        {
                            drm->drop_master();
                        }
                        catch (...)
                        {
                        }
                    }
                }
            }));

    auth_factory = std::make_unique<DRMNativePlatformAuthFactory>(*drm.front());
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgm::Platform::create_buffer_allocator()
{
    return make_module_ptr<mgm::BufferAllocator>(gbm->device, bypass_option_, mgm::BufferImportMethod::gbm_native_pixmap);
}

mir::UniqueModulePtr<mg::Display> mgm::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy, std::shared_ptr<GLConfig> const& gl_config)
{
    return make_module_ptr<mgm::Display>(
        drm,
        gbm,
        vt,
        bypass_option_,
        initial_conf_policy,
        gl_config,
        listener);
}

mg::NativeDisplayPlatform* mgm::Platform::native_display_platform()
{
    return auth_factory.get();
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgm::Platform::make_ipc_operations() const
{
    return make_module_ptr<mgm::IpcOperations>(drm.front());
}

mg::NativeRenderingPlatform* mgm::Platform::native_rendering_platform()
{
    return this;
}

MirServerEGLNativeDisplayType mgm::Platform::egl_native_display() const
{
    return gbm->device;
}

mgm::BypassOption mgm::Platform::bypass_option() const
{
    return bypass_option_;
}

std::vector<mir::ExtensionDescription> mgm::Platform::extensions() const
{
    return mgm::mesa_extensions();
}
