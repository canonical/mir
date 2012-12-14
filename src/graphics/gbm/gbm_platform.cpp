/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gbm_platform.h"

#include "mir/exception.h"
#include "gbm_buffer_allocator.h"
#include "gbm_display.h"
#include "gbm_display_reporter.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/logging/logger.h"
#include "mir/logging/dumb_console_logger.h"

#include <xf86drm.h>

#include <stdexcept>

namespace mgg=mir::graphics::gbm;
namespace mg=mir::graphics;
namespace ml=mir::logging;
namespace mc=mir::compositor;

namespace
{

// TODO not the best place, but convenient for refactoring
class NullDisplayListener : public mg::DisplayListener
{
  public:

    virtual void report_successful_setup_of_native_resources() {}
    virtual void report_successful_egl_make_current_on_construction() {}
    virtual void report_successful_egl_buffer_swap_on_construction() {}
    virtual void report_successful_drm_mode_set_crtc_on_construction() {}
    virtual void report_successful_display_construction() {}
};

struct GBMPlatformIPCPackage : public mg::PlatformIPCPackage
{
    GBMPlatformIPCPackage(int drm_auth_fd)
    {
        ipc_fds.push_back(drm_auth_fd);
    }

    ~GBMPlatformIPCPackage()
    {
        if (ipc_fds.size() > 0 && ipc_fds[0] >= 0)
            drmClose(ipc_fds[0]);
    }
};

}

mgg::GBMPlatform::GBMPlatform() :
    listener(std::make_shared<NullDisplayListener>())
{
    drm.setup();
    gbm.setup(drm);
}

mgg::GBMPlatform::GBMPlatform(std::shared_ptr<DisplayListener> const& listener) :
    listener(listener)
{
    drm.setup();
    gbm.setup(drm);
}

std::shared_ptr<mc::GraphicBufferAllocator> mgg::GBMPlatform::create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& buffer_initializer)
{
    return std::make_shared<mgg::GBMBufferAllocator>(this->shared_from_this(),
                                                     buffer_initializer);
}

std::shared_ptr<mg::Display> mgg::GBMPlatform::create_display()
{
    return std::make_shared<mgg::GBMDisplay>(
        this->shared_from_this(),
        listener);
}

std::shared_ptr<mg::PlatformIPCPackage> mgg::GBMPlatform::get_ipc_package()
{
    return std::make_shared<GBMPlatformIPCPackage>(drm.get_authenticated_fd());
}

void mgg::GBMPlatform::drm_auth_magic(drm_magic_t magic)
{
    drm.auth_magic(magic);
}

std::shared_ptr<mg::Platform> mg::create_platform()
{
    const std::shared_ptr<ml::Logger>  logger(std::make_shared<ml::DumbConsoleLogger>());
    return std::make_shared<mgg::GBMPlatform>(std::make_shared<mgg::GBMDisplayReporter>(logger));
}
