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

#include "platform.h"
#include "guest_platform.h"
#include "buffer_allocator.h"
#include "display.h"
#include "linux_virtual_terminal.h"
#include "ipc_operations.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/native_buffer.h"
#include "mir/emergency_cleanup_registry.h"
#include "mir/udev/wrapper.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mgmh = mgm::helpers;

mgm::Platform::Platform(std::shared_ptr<DisplayReport> const& listener,
                        std::shared_ptr<VirtualTerminal> const& vt,
                        EmergencyCleanupRegistry& emergency_cleanup_registry,
                        BypassOption bypass_option)
    : udev{std::make_shared<mir::udev::Context>()},
      drm{std::make_shared<mgmh::DRMHelper>(mgmh::DRMNodeToUse::card)},
      gbm{std::make_shared<mgmh::GBMHelper>()},
      listener{listener},
      vt{vt},
      bypass_option_{bypass_option}
{
    drm->setup(udev);
    gbm->setup(*drm);

    std::weak_ptr<VirtualTerminal> weak_vt = vt;
    std::weak_ptr<mgmh::DRMHelper> weak_drm = drm;
    emergency_cleanup_registry.add(
        [weak_vt,weak_drm]
        {
            if (auto const vt = weak_vt.lock())
                try { vt->restore(); } catch (...) {}

            if (auto const drm = weak_drm.lock())
                try { drm->drop_master(); } catch (...) {}
        });
}

std::shared_ptr<mg::GraphicBufferAllocator> mgm::Platform::create_buffer_allocator()
{
    return std::make_shared<mgm::BufferAllocator>(gbm->device, bypass_option_, mgm::BufferImportMethod::gbm_native_pixmap);
}

std::shared_ptr<mg::Display> mgm::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<GLConfig> const& gl_config)
{
    return std::make_shared<mgm::Display>(
        drm,
        gbm,
        vt,
        bypass_option_,
        initial_conf_policy,
        gl_config,
        listener);
}

std::shared_ptr<mg::PlatformIpcOperations> mgm::Platform::make_ipc_operations() const
{
    return std::make_shared<mgm::IpcOperations>(drm);
}

EGLNativeDisplayType mgm::Platform::egl_native_display() const
{
    return gbm->device;
}

mgm::BypassOption mgm::Platform::bypass_option() const
{
    return bypass_option_;
}

