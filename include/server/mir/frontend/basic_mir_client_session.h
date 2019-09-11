/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_BASIC_MIR_CLIENT_SESSION_H_
#define MIR_FRONTEND_BASIC_MIR_CLIENT_SESSION_H_

#include "mir/frontend/mir_client_session.h"
#include "mir/observer_registrar.h"

#include <unordered_map>
#include <mutex>
#include <atomic>

namespace mir
{
namespace scene
{
class Session;
}
namespace compositor
{
class BufferStream;
}
namespace graphics
{
class DisplayConfiguration;
class DisplayConfigurationObserver;
}
namespace frontend
{
class BasicMirClientSession : public MirClientSession
{
public:
    BasicMirClientSession(
        std::shared_ptr<scene::Session> session,
        std::shared_ptr<frontend::EventSink> const& event_sink,
        graphics::DisplayConfiguration const& initial_display_config,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar);

    ~BasicMirClientSession();

    auto name() const -> std::string override;
    auto get_surface(SurfaceId surface) const -> std::shared_ptr<Surface> override;

    auto create_surface(
        std::shared_ptr<shell::Shell> const& shell,
        scene::SurfaceCreationParameters const& params,
        std::shared_ptr<EventSink> const& sink) -> SurfaceId override;
    void destroy_surface(std::shared_ptr<shell::Shell> const& shell, SurfaceId surface) override;

    auto create_buffer_stream(graphics::BufferProperties const& props) -> BufferStreamId override;
    auto get_buffer_stream(BufferStreamId stream) const -> std::shared_ptr<BufferStream> override;
    void destroy_buffer_stream(BufferStreamId stream) override;

private:
    class DisplayConfigurationObserver;

    std::shared_ptr<scene::Session> const session_;
    std::shared_ptr<frontend::EventSink> const event_sink;
    std::weak_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const display_config_registrar;
    std::shared_ptr<DisplayConfigurationObserver> const display_config_observer;

    void send_display_config(graphics::DisplayConfiguration const& info);
};
}
}

#endif /* MIR_FRONTEND_BASIC_MIR_CLIENT_SESSION_H_ */
