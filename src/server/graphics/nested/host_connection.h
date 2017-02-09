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
#include "mir_toolkit/mir_native_buffer.h"
#include "mir/graphics/nested_context.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/displacement.h"
#include "mir/graphics/buffer_properties.h"

#include <memory>
#include <vector>
#include <functional>

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
class CursorImage;
class BufferProperties;
 
namespace nested
{
using UniqueInputConfig = std::unique_ptr<MirInputConfig, void(*)(MirInputConfig const*)>;

class HostStream;
class HostSurface;
class HostChain;
class HostSurfaceSpec;
class NativeBuffer;
class HostConnection : public NestedContext
{
public:
    virtual ~HostConnection() = default;

    virtual EGLNativeDisplayType egl_native_display() = 0;
    virtual std::shared_ptr<MirDisplayConfig> create_display_config() = 0;
    virtual void set_display_config_change_callback(std::function<void()> const& cb) = 0;
    virtual void apply_display_config(MirDisplayConfig&) = 0;
    virtual std::unique_ptr<HostStream> create_stream(BufferProperties const& properties) const = 0;
    virtual std::unique_ptr<HostChain> create_chain() const = 0;
    virtual std::unique_ptr<HostSurfaceSpec> create_surface_spec() = 0;
    virtual std::shared_ptr<HostSurface> create_surface(
        std::shared_ptr<HostStream> const& stream,
        geometry::Displacement stream_displacement,
        graphics::BufferProperties properties,
        char const* name, uint32_t output_id) = 0;

    virtual void set_cursor_image(CursorImage const& image) = 0;
    virtual void hide_cursor() = 0;
    virtual auto graphics_platform_library() -> std::string = 0;

    virtual UniqueInputConfig create_input_device_config() = 0;
    virtual void set_input_device_change_callback(std::function<void(UniqueInputConfig)> const& cb) = 0;
    virtual void set_input_event_callback(std::function<void(MirEvent const&, mir::geometry::Rectangle const&)> const& cb) = 0;
    virtual void emit_input_event(MirEvent const& event, mir::geometry::Rectangle const& source_frame) = 0;
    virtual std::shared_ptr<NativeBuffer> create_buffer(graphics::BufferProperties const&) = 0;
    virtual std::shared_ptr<NativeBuffer> create_buffer(mir::geometry::Size, MirPixelFormat format) = 0;
    virtual std::shared_ptr<NativeBuffer> create_buffer(geometry::Size, uint32_t format, uint32_t flags) = 0;
    virtual bool supports_passthrough(graphics::BufferUsage) = 0;

protected:
    HostConnection() = default;
    HostConnection(HostConnection const&) = delete;
    HostConnection& operator=(HostConnection const&) = delete;
};

}
}
}
#endif // MIR_GRAPHICS_NESTED_HOST_CONNECTION_H_
