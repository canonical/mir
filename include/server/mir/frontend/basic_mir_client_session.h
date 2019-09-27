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
#include "mir/scene/output_properties_cache.h"

#include <unordered_map>
#include <mutex>
#include <atomic>

namespace mir
{
namespace scene
{
class Session;
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
    auto frontend_surface(SurfaceId id) const -> std::shared_ptr<Surface> override;
    auto scene_surface(SurfaceId id) const -> std::shared_ptr<scene::Surface> override;

    auto create_surface(
        std::shared_ptr<shell::Shell> const& shell,
        scene::SurfaceCreationParameters const& params,
        std::shared_ptr<frontend::EventSink> const& sink) -> frontend::SurfaceId override;
    void destroy_surface(std::shared_ptr<shell::Shell> const& shell, frontend::SurfaceId id) override;

    auto create_buffer_stream(graphics::BufferProperties const& props) -> BufferStreamId override;
    auto buffer_stream(BufferStreamId id) const -> std::shared_ptr<compositor::BufferStream> override;
    void destroy_buffer_stream(BufferStreamId id) override;

private:
    class DisplayConfigurationObserver;
    typedef std::unordered_map<SurfaceId, std::weak_ptr<scene::Surface>> Surfaces;
    typedef std::unordered_map<BufferStreamId, std::weak_ptr<compositor::BufferStream>> Streams;

    std::shared_ptr<scene::Session> const session_;
    std::shared_ptr<frontend::EventSink> const event_sink;
    std::weak_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const display_config_registrar;
    std::shared_ptr<DisplayConfigurationObserver> const display_config_observer;
    scene::OutputPropertiesCache output_cache;
    Surfaces surfaces;
    Streams streams;
    std::atomic<int> next_id;
    std::mutex mutable surfaces_and_streams_mutex;

    void send_display_config(graphics::DisplayConfiguration const& info);

    auto checked_find(frontend::SurfaceId id) const -> Surfaces::const_iterator;
    auto checked_find(std::shared_ptr<scene::Surface> const& surface) const -> Surfaces::const_iterator; ///< O(n)
    auto checked_find(frontend::BufferStreamId id) const -> Streams::const_iterator;
};
}
}

#endif /* MIR_FRONTEND_BASIC_MIR_CLIENT_SESSION_H_ */
