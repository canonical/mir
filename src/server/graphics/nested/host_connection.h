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
#include "mir/graphics/nested_context.h"

#include <memory>
#include <vector>
#include <functional>

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
class CursorImage;
 
namespace nested
{
class HostSurface;
class HostConnection : public NestedContext
{
public:
    virtual ~HostConnection() = default;

    virtual EGLNativeDisplayType egl_native_display() = 0;
    virtual std::shared_ptr<MirDisplayConfiguration> create_display_config() = 0;
    virtual void set_display_config_change_callback(std::function<void()> const& cb) = 0;
    virtual void apply_display_config(MirDisplayConfiguration&) = 0;
    virtual std::shared_ptr<HostSurface> create_surface(
        int width, int height, MirPixelFormat pf, char const* name,
        MirBufferUsage usage, uint32_t output_id) = 0;

    virtual void set_cursor_image(CursorImage const& image) = 0;
    virtual void hide_cursor() = 0;

protected:
    HostConnection() = default;
    HostConnection(HostConnection const&) = delete;
    HostConnection& operator=(HostConnection const&) = delete;
};

}
}
}
#endif // MIR_GRAPHICS_NESTED_HOST_CONNECTION_H_
