/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SCENE_APPLICATION_SESSION_H_
#define MIR_SCENE_APPLICATION_SESSION_H_

#include "mir/shell/session.h"

#include <map>

namespace mir
{
namespace frontend
{
class EventSink;
}
namespace shell
{
class SurfaceFactory;
class Surface;
class SessionListener;
}

namespace scene
{
class SnapshotStrategy;

class ApplicationSession : public shell::Session
{
public:
    ApplicationSession(
        std::shared_ptr<shell::SurfaceFactory> const& surface_factory,
        pid_t pid,
        std::string const& session_name,
        std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
        std::shared_ptr<shell::SessionListener> const& session_listener,
        std::shared_ptr<frontend::EventSink> const& sink);

    ~ApplicationSession();

    frontend::SurfaceId create_surface(shell::SurfaceCreationParameters const& params);
    void destroy_surface(frontend::SurfaceId surface);
    std::shared_ptr<frontend::Surface> get_surface(frontend::SurfaceId surface) const;

    void take_snapshot(shell::SnapshotCallback const& snapshot_taken);
    std::shared_ptr<shell::Surface> default_surface() const;

    std::string name() const;
    pid_t process_id() const override;

    void force_requests_to_complete();

    void hide();
    void show();

    void send_display_config(graphics::DisplayConfiguration const& info);
    int configure_surface(frontend::SurfaceId id, MirSurfaceAttrib attrib, int value);

    void set_lifecycle_state(MirLifecycleState state);

    shell::Session* get_parent() const override;
    void set_parent(shell::Session* parent) override;
    std::shared_ptr<SessionContainer> get_children() const override;

    void begin_trust_session(std::shared_ptr<shell::TrustSession> const& trust_session,
                             std::vector<std::shared_ptr<shell::Session>> const& trusted_children) override;
    void add_trusted_child(std::shared_ptr<Session> const& session) override;
    void end_trust_session() override;

    std::shared_ptr<shell::TrustSession> get_trust_session() const override;
    void set_trust_session(std::shared_ptr<shell::TrustSession> const& trust_session) override;


protected:
    ApplicationSession(ApplicationSession const&) = delete;
    ApplicationSession& operator=(ApplicationSession const&) = delete;

private:
    std::shared_ptr<shell::SurfaceFactory> const surface_factory;
    pid_t const pid;
    std::string const session_name;
    std::shared_ptr<SnapshotStrategy> const snapshot_strategy;
    std::shared_ptr<shell::SessionListener> const session_listener;
    std::shared_ptr<frontend::EventSink> const event_sink;

    frontend::SurfaceId next_id();

    std::atomic<int> next_surface_id;

    typedef std::map<frontend::SurfaceId, std::shared_ptr<shell::Surface>> Surfaces;
    Surfaces::const_iterator checked_find(frontend::SurfaceId id) const;
    std::mutex mutable surfaces_mutex;
    Surfaces surfaces;

    // trust sessions
    std::mutex mutable mutex;
    std::shared_ptr<shell::TrustSession> trust_session;
    Session* parent;
    std::shared_ptr<SessionContainer> children;
};

}
} // namespace mir

#endif // MIR_SCENE_APPLICATION_SESSION_H_
