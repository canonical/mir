/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SHELL_APPLICATION_MANAGER_H_
#define MIR_SHELL_APPLICATION_MANAGER_H_

#include "mir/frontend/surface_id.h"
#include "mir/frontend/shell.h"

#include <mutex>
#include <memory>
#include <vector>

namespace mir
{

namespace frontend
{
class EventSink;
}
/// Management of sessions and surfaces
namespace shell
{
class SurfaceFactory;
class SessionContainer;
class FocusSetter;
class Session;
class InputRegistrar;
class SnapshotStrategy;
class SessionListener;
struct SurfaceCreationParameters;
class DisplayChanger;

class SessionManager : public frontend::Shell
{
public:
    explicit SessionManager(std::shared_ptr<SurfaceFactory> const& surface_factory,
                            std::shared_ptr<SessionContainer> const& app_container,
                            std::shared_ptr<FocusSetter> const& focus_setter,
                            std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
                            std::shared_ptr<SessionListener> const& session_listener,
                            std::shared_ptr<DisplayChanger> const& display_changer);
    virtual ~SessionManager();

    virtual std::shared_ptr<frontend::Session> open_session(
        std::string const& name, std::shared_ptr<frontend::EventSink> const& sink);
    virtual void close_session(std::shared_ptr<frontend::Session> const& session);

    virtual void handle_display_configuration(std::shared_ptr<frontend::Session> const& session); 
    virtual void handle_surface_created(std::shared_ptr<frontend::Session> const& session);

    virtual frontend::SurfaceId create_surface_for(std::shared_ptr<frontend::Session> const& session,
                                             shell::SurfaceCreationParameters const& params);

protected:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

private:
    std::shared_ptr<SurfaceFactory> const surface_factory;
    std::shared_ptr<SessionContainer> const app_container;
    std::shared_ptr<FocusSetter> const focus_setter;
    std::shared_ptr<SnapshotStrategy> const snapshot_strategy;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<DisplayChanger> const display_changer;
};

}
}

#endif // MIR_SHELL_APPLICATION_MANAGER_H_
