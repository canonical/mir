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

#include <string>
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

using UniqueInputConfig = std::unique_ptr<MirInputConfig, void(*)(MirInputConfig const*)>;

class MirClientHostConnection : public HostConnection, public input::InputDeviceHub
{
public:
    MirClientHostConnection(std::string const& host_socket,
                            std::string const& name,
                            std::shared_ptr<msh::HostLifecycleEventListener> const& host_lifecycle_event_listener,
                            std::shared_ptr<frontend::EventSink> const& sink,
                            std::shared_ptr<ServerActionQueue> const& observer_queue);
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

    // InputDeviceHub
    void add_observer(std::shared_ptr<input::InputDeviceObserver> const&) override;
    void remove_observer(std::weak_ptr<input::InputDeviceObserver> const&) override;
    void for_each_input_device(std::function<void(input::Device const& device)> const& callback) override;

private:
    void update_input_devices();
    std::mutex surfaces_mutex;

    MirConnection* const mir_connection;
    std::function<void()> conf_change_callback;
    std::shared_ptr<msh::HostLifecycleEventListener> const host_lifecycle_event_listener;

    std::vector<HostSurface*> surfaces;

    std::shared_ptr<frontend::EventSink> const sink;
    std::shared_ptr<mir::ServerActionQueue> const observer_queue;
    std::vector<std::shared_ptr<input::InputDeviceObserver>> observers;
    std::mutex devices_guard;
    std::vector<std::shared_ptr<input::Device>> devices;
    UniqueInputConfig config;

};

}
}
}

#endif /* MIR_GRAPHICS_NESTED_MIR_CLIENT_HOST_CONNECTION_H_ */
