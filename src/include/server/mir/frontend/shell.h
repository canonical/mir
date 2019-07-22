/*
 * Copyright © 2012-2015 Canonical Ltd.
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

#ifndef MIR_FRONTEND_SHELL_H_
#define MIR_FRONTEND_SHELL_H_

#include "mir/frontend/surface_id.h"
#include "mir/optional_value.h"
#include "mir_toolkit/common.h"

#include <sys/types.h>

#include <memory>
#include <vector>

namespace mir
{
namespace scene
{
struct SurfaceCreationParameters;
struct PromptSessionCreationParameters;
class Surface;
class Observer;
}
namespace shell { class SurfaceSpecification; }

namespace frontend
{
class EventSink;
class Session;
class PromptSession;

class Shell
{
public:
    virtual ~Shell() = default;

    virtual std::shared_ptr<Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<EventSink> const& sink) = 0;

    virtual void close_session(std::shared_ptr<Session> const& session)  = 0;

    virtual std::shared_ptr<PromptSession> start_prompt_session_for(std::shared_ptr<Session> const& session,
                                                                  scene::PromptSessionCreationParameters const& params) = 0;
    virtual void add_prompt_provider_for(std::shared_ptr<PromptSession> const& prompt_session,
                                                                  std::shared_ptr<Session> const& session) = 0;
    virtual void stop_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) = 0;

    virtual SurfaceId create_surface(
        std::shared_ptr<Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::shared_ptr<EventSink> const& sink) = 0;
    virtual void modify_surface(std::shared_ptr<Session> const& session, SurfaceId surface, shell::SurfaceSpecification const& modifications) = 0;
    virtual void destroy_surface(std::shared_ptr<Session> const& session, SurfaceId surface) = 0;

    virtual std::string persistent_id_for(std::shared_ptr<Session> const& session, SurfaceId surface) = 0;
    virtual std::shared_ptr<scene::Surface> surface_for_id(std::string const& serialised_id) = 0;

    virtual int set_surface_attribute(
        std::shared_ptr<Session> const& session,
        SurfaceId surface_id,
        MirWindowAttrib attrib,
        int value) = 0;

    virtual int get_surface_attribute(
        std::shared_ptr<Session> const& session,
        SurfaceId surface_id,
        MirWindowAttrib attrib) = 0;

    enum class UserRequest
    {
        drag_and_drop,
        move,
        activate,
        resize,
    };

    virtual void request_operation(
        std::shared_ptr<Session> const& session,
        SurfaceId surface_id, uint64_t timestamp,
        UserRequest request,
        optional_value<uint32_t> hint) = 0;

    void request_operation(
        std::shared_ptr<Session> const& session,
        SurfaceId surface_id, uint64_t timestamp,
        UserRequest request)
    {
        request_operation(session, surface_id, timestamp, request, optional_value<uint32_t>{});
    }

protected:
    Shell() = default;
    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;
};

}
}

#endif // MIR_FRONTEND_SHELL_H_
