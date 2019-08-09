/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_SHELL_WRAPPER_H_
#define MIR_FRONTEND_SHELL_WRAPPER_H_

#include "mir/frontend/shell.h"

namespace mir
{
namespace frontend
{
class ShellWrapper : public Shell
{
public:
    explicit ShellWrapper(std::shared_ptr<Shell> const& wrapped) :
        wrapped(wrapped) {}

    virtual ~ShellWrapper() = default;

    std::shared_ptr<MirClientSession> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<EventSink> const& sink) override;

    void close_session(std::shared_ptr<MirClientSession> const& session) override;

    auto scene_session_for(
        std::shared_ptr<MirClientSession> const& session) -> std::shared_ptr<scene::Session> override;

    std::shared_ptr<PromptSession> start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params) override;

    void add_prompt_provider_for(
        std::shared_ptr<PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session) override;

    void stop_prompt_session(
        std::shared_ptr<PromptSession> const& prompt_session) override;

    SurfaceId create_surface(
        std::shared_ptr<MirClientSession> const& session,
        scene::SurfaceCreationParameters const& params,
        std::shared_ptr<EventSink> const& sink) override;

    void modify_surface(std::shared_ptr<MirClientSession> const& session, SurfaceId surface, shell::SurfaceSpecification const& modifications) override;

    void destroy_surface(std::shared_ptr<MirClientSession> const& session, SurfaceId surface) override;

    std::string persistent_id_for(std::shared_ptr<MirClientSession> const& session, SurfaceId surface) override;

    std::shared_ptr<scene::Surface> surface_for_id(std::string const& serialised_id) override;

    int set_surface_attribute(
        std::shared_ptr<MirClientSession> const& session,
        SurfaceId surface_id,
        MirWindowAttrib attrib,
        int value) override;

    int get_surface_attribute(
        std::shared_ptr<MirClientSession> const& session,
        SurfaceId surface_id,
        MirWindowAttrib attrib) override;

    void request_operation(
        std::shared_ptr<MirClientSession> const& session,
        SurfaceId surface_id,
        uint64_t timestamp,
        UserRequest request,
        optional_value <uint32_t> hint) override;

protected:
    std::shared_ptr<Shell> const wrapped;
};
}
}

#endif /* MIR_FRONTEND_SHELL_WRAPPER_H_ */
