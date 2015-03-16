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

#include "frontend_shell.h"
#include "mir/shell/shell.h"

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/prompt_session.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell::detail;

std::shared_ptr<mf::Session> msh::FrontendShell::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    return wrapped->open_session(client_pid, name, sink);
}

void msh::FrontendShell::close_session(std::shared_ptr<mf::Session> const& session)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);

    wrapped->close_session(scene_session);
}

std::shared_ptr<mf::PromptSession> msh::FrontendShell::start_prompt_session_for(
    std::shared_ptr<mf::Session> const& session,
    ms::PromptSessionCreationParameters const& params)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);

    return wrapped->start_prompt_session_for(scene_session, params);
}

void msh::FrontendShell::add_prompt_provider_for(
    std::shared_ptr<mf::PromptSession> const& prompt_session,
    std::shared_ptr<mf::Session> const& session)
{
    auto const scene_prompt_session = std::dynamic_pointer_cast<ms::PromptSession>(prompt_session);
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);
    wrapped->add_prompt_provider_for(scene_prompt_session, scene_session);
}

void msh::FrontendShell::stop_prompt_session(std::shared_ptr<mf::PromptSession> const& prompt_session)
{
    auto const scene_prompt_session = std::dynamic_pointer_cast<ms::PromptSession>(prompt_session);
    wrapped->stop_prompt_session(scene_prompt_session);
}

mf::SurfaceId msh::FrontendShell::create_surface(std::shared_ptr<mf::Session> const& session, ms::SurfaceCreationParameters const& params)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);

    auto populated_params = params;

    if (populated_params.parent_id.is_set())
        populated_params.parent = scene_session->surface(populated_params.parent_id.value());

    return wrapped->create_surface(scene_session, populated_params);
}

void msh::FrontendShell::destroy_surface(std::shared_ptr<mf::Session> const& session, mf::SurfaceId surface)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);
    wrapped->destroy_surface(scene_session, surface);
}

int msh::FrontendShell::set_surface_attribute(
    std::shared_ptr<mf::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib,
    int value)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);
    auto const surface = scene_session->surface(surface_id);
    return wrapped->set_surface_attribute(scene_session, surface, attrib, value);
}

int msh::FrontendShell::get_surface_attribute(
    std::shared_ptr<mf::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);
    auto const surface = scene_session->surface(surface_id);
    return wrapped->get_surface_attribute(surface, attrib);
}
