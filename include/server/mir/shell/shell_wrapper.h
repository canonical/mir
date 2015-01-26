/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SHELL_SHELL_WRAPPER_H_
#define MIR_SHELL_SHELL_WRAPPER_H_

#include "mir/shell/shell.h"

namespace mir
{
namespace shell
{
class ShellWrapper : public Shell
{
public:
    explicit ShellWrapper(std::shared_ptr<Shell> const& wrapped);

    void focus_next() override;

    std::weak_ptr<scene::Session> focussed_application() const override;

    void set_focus_to(std::shared_ptr<scene::Session> const& focus) override;

    virtual std::shared_ptr<scene::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink);

    virtual void close_session(std::shared_ptr<scene::Session> const& session);

    virtual void handle_surface_created(std::shared_ptr<scene::Session> const& session);

    virtual std::shared_ptr<scene::PromptSession> start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params);

    virtual void add_prompt_provider_for(
        std::shared_ptr<scene::PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session);

    virtual void stop_prompt_session(std::shared_ptr<scene::PromptSession> const& prompt_session);

    virtual frontend::SurfaceId create_surface(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params);

    virtual void destroy_surface(std::shared_ptr<scene::Session> const& session, frontend::SurfaceId surface);

    virtual int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        frontend::SurfaceId surface_id,
        MirSurfaceAttrib attrib,
        int value);

    virtual int get_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        frontend::SurfaceId surface_id,
        MirSurfaceAttrib attrib);

protected:
    std::shared_ptr<Shell> const wrapped;
};
}
}

#endif /* MIR_SHELL_SHELL_WRAPPER_H_ */
