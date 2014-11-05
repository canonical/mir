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

#include "mir/scene/session.h"

#include <atomic>
#include <map>
#include <mutex>

namespace mir
{
namespace frontend
{
class EventSink;
}

namespace scene
{
class SessionListener;
class Surface;
class SurfaceCoordinator;
class SnapshotStrategy;

class ApplicationSession : public Session
{
public:
    ApplicationSession(
        std::shared_ptr<SurfaceCoordinator> const& surface_coordinator,
        pid_t pid,
        std::string const& session_name,
        std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
        std::shared_ptr<SessionListener> const& session_listener,
        std::shared_ptr<frontend::EventSink> const& sink);

    ~ApplicationSession();

    frontend::SurfaceId create_surface(SurfaceCreationParameters const& params) override;
    void destroy_surface(frontend::SurfaceId surface) override;
    std::shared_ptr<frontend::Surface> get_surface(frontend::SurfaceId surface) const override;

    void take_snapshot(SnapshotCallback const& snapshot_taken) override;
    std::shared_ptr<Surface> default_surface() const override;

    std::string name() const override;
    pid_t process_id() const override;

    void force_requests_to_complete() override;

    void hide() override;
    void show() override;

    void send_display_config(graphics::DisplayConfiguration const& info) override;

    void set_lifecycle_state(MirLifecycleState state) override;

    void start_prompt_session() override;
    void stop_prompt_session() override;

protected:
    ApplicationSession(ApplicationSession const&) = delete;
    ApplicationSession& operator=(ApplicationSession const&) = delete;

private:
    std::shared_ptr<SurfaceCoordinator> const surface_coordinator;
    pid_t const pid;
    std::string const session_name;
    std::shared_ptr<SnapshotStrategy> const snapshot_strategy;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<frontend::EventSink> const event_sink;

    frontend::SurfaceId next_id();

    std::atomic<int> next_surface_id;

    typedef std::map<frontend::SurfaceId, std::shared_ptr<Surface>> Surfaces;
    Surfaces::const_iterator checked_find(frontend::SurfaceId id) const;
    std::mutex mutable surfaces_mutex;
    Surfaces surfaces;
};

}
} // namespace mir

#endif // MIR_SCENE_APPLICATION_SESSION_H_
