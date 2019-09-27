/*
 * Copyright © 2012-2019 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SCENE_APPLICATION_MANAGER_H_
#define MIR_SCENE_APPLICATION_MANAGER_H_

#include "mir/scene/session_coordinator.h"
#include "mir/scene/session_listener.h"
#include "mir/observer_registrar.h"

#include <memory>
#include <vector>

namespace mir
{
namespace graphics
{
class DisplayConfiguration;
class Display;
class GraphicBufferAllocator;
class DisplayConfigurationObserver;
}

namespace shell { class SurfaceStack; }

namespace scene
{
class SessionContainer;
class SessionEventSink;
class SessionListener;
class SnapshotStrategy;
class SurfaceStack;
class PromptSessionManager;
class BufferStreamFactory;
class SurfaceFactory;
class ApplicationNotRespondingDetector;

class SessionManager : public SessionCoordinator
{
public:
    SessionManager(
        std::shared_ptr<shell::SurfaceStack> const& surface_stack,
        std::shared_ptr<SurfaceFactory> const& surface_factory,
        std::shared_ptr<BufferStreamFactory> const& buffer_stream_factory,
        std::shared_ptr<SessionContainer> const& app_container,
        std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
        std::shared_ptr<SessionEventSink> const& session_event_sink,
        std::shared_ptr<SessionListener> const& session_listener,
        std::shared_ptr<graphics::Display const> const& display,
        std::shared_ptr<ApplicationNotRespondingDetector> const& anr_detector,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar);

    virtual ~SessionManager() noexcept;

    auto open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) -> std::shared_ptr<Session> override;

    void close_session(std::shared_ptr<Session> const& session) override;

    auto successor_of(std::shared_ptr<Session> const&) const -> std::shared_ptr<Session> override;
    auto predecessor_of(std::shared_ptr<Session> const&) const -> std::shared_ptr<Session> override;

    void set_focus_to(std::shared_ptr<Session> const& focus) override;
    void unset_focus() override;

    void add_listener(std::shared_ptr<SessionListener> const& listener) override;
    void remove_listener(std::shared_ptr<SessionListener> const& listener) override;

protected:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

private:
    struct SessionObservers;
    std::shared_ptr<SessionObservers> const observers;
    std::shared_ptr<shell::SurfaceStack> const surface_stack;
    std::shared_ptr<SurfaceFactory> const surface_factory;
    std::shared_ptr<BufferStreamFactory> const buffer_stream_factory;
    std::shared_ptr<SessionContainer> const app_container;
    std::shared_ptr<SnapshotStrategy> const snapshot_strategy;
    std::shared_ptr<SessionEventSink> const session_event_sink;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<graphics::Display const> const display;
    std::shared_ptr<ApplicationNotRespondingDetector> const anr_detector;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> display_config_registrar;
};

}
}

#endif // MIR_SCENE_APPLICATION_MANAGER_H_
