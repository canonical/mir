/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_MIR_CLIENT_HOST_CONNECTION_H_
#define MIR_GRAPHICS_NESTED_MIR_CLIENT_HOST_CONNECTION_H_

#include "host_connection.h"
#include "mir/shell/host_lifecycle_event_listener.h"
#include "mir/input/input_device_hub.h"
#include "mir/geometry/size.h"
#include "mir/geometry/displacement.h"
#include "mir/graphics/cursor_image.h"

#include <string>
#include <vector>
#include <mutex>

struct MirConnection;

namespace msh = mir::shell;

namespace mir
{
class ServerActionQueue;
namespace frontend
{
class EventSink;
}
namespace input
{
class InputDeviceObserver;
class Device;
}
namespace graphics
{
namespace nested
{

class MirClientHostConnection : public HostConnection
{
public:
    MirClientHostConnection(std::string const& host_socket,
                            std::string const& name,
                            std::shared_ptr<msh::HostLifecycleEventListener> const& host_lifecycle_event_listener);
    ~MirClientHostConnection();

    std::vector<int> platform_fd_items() override;
    EGLNativeDisplayType egl_native_display() override;
    std::shared_ptr<MirDisplayConfiguration> create_display_config() override;
    std::shared_ptr<HostSurface> create_surface(
        int width, int height, MirPixelFormat pf, char const* name,
        MirBufferUsage usage, uint32_t output_id) override;
    void set_display_config_change_callback(std::function<void()> const& cb) override;
    void apply_display_config(MirDisplayConfiguration&) override;

    void set_cursor_image(CursorImage const& image) override;
    void hide_cursor() override;
    auto graphics_platform_library() -> std::string override;

    virtual PlatformOperationMessage platform_operation(
        unsigned int op, PlatformOperationMessage const& request) override;

    UniqueInputConfig create_input_device_config() override;
    void set_input_device_change_callback(std::function<void(UniqueInputConfig)> const& cb) override;
    void set_input_event_callback(std::function<void(MirEvent const&, mir::geometry::Rectangle const&)> const& cb) override;
    void emit_input_event(MirEvent const& cb, mir::geometry::Rectangle const& source_frame) override;

private:
    void update_input_config(UniqueInputConfig input_config);
    std::mutex surfaces_mutex;

    MirConnection* const mir_connection;
    std::function<void()> conf_change_callback;
    std::shared_ptr<msh::HostLifecycleEventListener> const host_lifecycle_event_listener;

    std::vector<HostSurface*> surfaces;

    std::function<void(UniqueInputConfig)> input_config_callback;
    std::function<void(MirEvent const&, mir::geometry::Rectangle const&)> event_callback;

    struct NestedCursorImage : graphics::CursorImage
    {
        NestedCursorImage() = default;
        NestedCursorImage(graphics::CursorImage const& other);
        NestedCursorImage& operator=(graphics::CursorImage const& other);

        void const* as_argb_8888() const override;
        geometry::Size size() const override;
        geometry::Displacement hotspot() const override;
    private:
        geometry::Displacement hotspot_;
        geometry::Size size_;
        std::vector<uint8_t> buffer;
    };
    NestedCursorImage stored_cursor_image;
};

}
}
}

#endif /* MIR_GRAPHICS_NESTED_MIR_CLIENT_HOST_CONNECTION_H_ */
