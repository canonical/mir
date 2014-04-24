/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_HOST_CONNECTION_H_
#define MIR_GRAPHICS_NESTED_HOST_CONNECTION_H_

#include "mir_toolkit/client_types.h"

#include <memory>
#include <vector>
#include <functional>

#include <EGL/egl.h>

struct gbm_device;

namespace mir
{
namespace graphics
{
namespace nested
{

class HostSurface
{
public:
    virtual ~HostSurface() = default;

    virtual EGLNativeWindowType egl_native_window() = 0;
    virtual void set_event_handler(MirEventDelegate const* handler) = 0;

protected:
    HostSurface() = default;
    HostSurface(HostSurface const&) = delete;
    HostSurface& operator=(HostSurface const&) = delete;
};

class HostConnection
{
public:
    virtual ~HostConnection() = default;

    virtual std::vector<int> platform_fd_items() = 0;
    virtual EGLNativeDisplayType egl_native_display() = 0;
    virtual std::shared_ptr<MirDisplayConfiguration> create_display_config() = 0;
    virtual void set_display_config_change_callback(std::function<void()> const& cb) = 0;
    virtual void apply_display_config(MirDisplayConfiguration&) = 0;
    virtual std::shared_ptr<HostSurface> create_surface(MirSurfaceParameters const&) = 0;

    virtual void drm_auth_magic(int magic) = 0;
    virtual void drm_set_gbm_device(struct gbm_device* dev) = 0;

protected:
    HostConnection() = default;
    HostConnection(HostConnection const&) = delete;
    HostConnection& operator=(HostConnection const&) = delete;
};

}
}
}
#endif // MIR_GRAPHICS_NESTED_HOST_CONNECTION_H_
