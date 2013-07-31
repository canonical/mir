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
#include "mir/shell/focus_controller.h"

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
class FocusSequence;
class FocusSetter;
class Session;
class InputRegistrar;
class SnapshotStrategy;
class SessionListener;
struct SurfaceCreationParameters;

class SessionManager : public frontend::Shell, public shell::FocusController
{
public:
    explicit SessionManager(std::shared_ptr<SurfaceFactory> const& surface_factory,
                            std::shared_ptr<SessionContainer> const& app_container,
                            std::shared_ptr<FocusSequence> const& focus_sequence,
                            std::shared_ptr<FocusSetter> const& focus_setter,
                            std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
                            std::shared_ptr<SessionListener> const& session_listener);
    virtual ~SessionManager();

    virtual std::shared_ptr<frontend::Session> open_session(
        std::string const& name, std::shared_ptr<frontend::EventSink> const& sink);
    virtual void close_session(std::shared_ptr<frontend::Session> const& session);

    frontend::SurfaceId create_surface_for(std::shared_ptr<frontend::Session> const& session,
                                 SurfaceCreationParameters const& params);

    void focus_next();
    std::weak_ptr<Session> focussed_application() const;
    void set_focus_to(std::shared_ptr<Session> const& focus);

protected:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

private:
    std::shared_ptr<SurfaceFactory> const surface_factory;
    std::shared_ptr<SessionContainer> const app_container;
    std::shared_ptr<FocusSequence> const focus_sequence;
    std::shared_ptr<FocusSetter> const focus_setter;
    std::shared_ptr<SnapshotStrategy> const snapshot_strategy;
    std::shared_ptr<SessionListener> const session_listener;

    std::mutex mutex;
    std::weak_ptr<Session> focus_application;

    void set_focus_to_locked(std::unique_lock<std::mutex> const& lock, std::shared_ptr<Session> const& next_focus);
};

}
}

#endif // MIR_SHELL_APPLICATION_MANAGER_H_
