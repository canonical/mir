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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SHELL_APPLICATION_SESSION_H_
#define MIR_SHELL_APPLICATION_SESSION_H_

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
class SnapshotStrategy;
class SessionListener;

class ApplicationSession : public Session
{
public:
    ApplicationSession(
        std::shared_ptr<SurfaceFactory> const& surface_factory,
        std::string const& session_name,
        std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
        std::shared_ptr<SessionListener> const& session_listener,
        std::shared_ptr<frontend::EventSink> const& sink);

    ~ApplicationSession();

    frontend::SurfaceId create_surface(shell::SurfaceCreationParameters const& params);
    void destroy_surface(frontend::SurfaceId surface);
    std::shared_ptr<frontend::Surface> get_surface(frontend::SurfaceId surface) const;

    void take_snapshot(SnapshotCallback const& snapshot_taken);
    std::shared_ptr<Surface> default_surface() const;

    std::string name() const;

    void force_requests_to_complete();

    void hide();
    void show();

    void send_display_config(graphics::DisplayConfiguration const& info);
    int configure_surface(frontend::SurfaceId id, MirSurfaceAttrib attrib, int value);

protected:
    ApplicationSession(ApplicationSession const&) = delete;
    ApplicationSession& operator=(ApplicationSession const&) = delete;

private:
    std::shared_ptr<SurfaceFactory> const surface_factory;
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

#endif // MIR_SHELL_APPLICATION_SESSION_H_
