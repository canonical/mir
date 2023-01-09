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

#include "platform.h"
#include "buffer_allocator.h"
#include "display.h"
#include "mir/console_services.h"
#include "mir/emergency_cleanup_registry.h"
#include "mir/udev/wrapper.h"

#define MIR_LOG_COMPONENT "platform-graphics-gbm-kms"
#include "mir/log.h"

#include <wayland-server-protocol.h>

namespace mg = mir::graphics;
namespace mgg = mg::gbm;
namespace mgmh = mgg::helpers;

mgg::Platform::Platform(std::shared_ptr<DisplayReport> const& listener,
                        std::shared_ptr<ConsoleServices> const& vt,
                        EmergencyCleanupRegistry&,
                        BypassOption bypass_option,
                        std::unique_ptr<Quirks> quirks)
    : udev{std::make_shared<mir::udev::Context>()},
      drm{helpers::DRMHelper::open_all_devices(udev, *vt, *quirks)},
      // We assume the first DRM device is the boot GPU, and arbitrarily pick it as our
      // shell renderer.
      //
      // TODO: expose multiple rendering GPUs to the shell.
      gbm{std::make_shared<mgmh::GBMHelper>(drm.front()->fd)},
      listener{listener},
      vt{vt},
      bypass_option_{bypass_option}
{
}

mgg::RenderingPlatform::RenderingPlatform(
    mir::udev::Device const&,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const&)
{
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgg::RenderingPlatform::create_buffer_allocator(
    mg::Display const& output)
{
    return make_module_ptr<mgg::BufferAllocator>(output);
}

mir::UniqueModulePtr<mg::Display> mgg::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy, std::shared_ptr<GLConfig> const& gl_config)
{
    return make_module_ptr<mgg::Display>(
        drm,
        gbm,
        vt,
        bypass_option_,
        initial_conf_policy,
        gl_config,
        listener);
}

mgg::BypassOption mgg::Platform::bypass_option() const
{
    return bypass_option_;
}
