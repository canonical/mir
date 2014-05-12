/*
 * Copyright © 2014 Canonical Ltd.
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

#include "default_ipc_factory.h"

#include "session_mediator.h"
#include "unauthorized_display_changer.h"
#include "unauthorized_screencast.h"
#include "resource_cache.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/graphics/graphic_buffer_allocator.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;

mf::DefaultIpcFactory::DefaultIpcFactory(
    std::shared_ptr<Shell> const& shell,
    std::shared_ptr<SessionMediatorReport> const& sm_report,
    std::shared_ptr<mg::Platform> const& graphics_platform,
    std::shared_ptr<DisplayChanger> const& display_changer,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<Screencast> const& screencast,
    std::shared_ptr<SessionAuthorizer> const& session_authorizer) :
    shell(shell),
    sm_report(sm_report),
    cache(std::make_shared<ResourceCache>()),
    graphics_platform(graphics_platform),
    display_changer(display_changer),
    buffer_allocator(buffer_allocator),
    screencast(screencast),
    session_authorizer(session_authorizer)
{
}

std::shared_ptr<mf::detail::DisplayServer> mf::DefaultIpcFactory::make_ipc_server(
    SessionCredentials const& creds,
    std::shared_ptr<EventSink> const& sink,
    ConnectionContext const& connection_context)
{
    std::shared_ptr<DisplayChanger> changer;
    std::shared_ptr<Screencast> effective_screencast;

    if (session_authorizer->configure_display_is_allowed(creds))
    {
        changer = display_changer;
    }
    else
    {
        changer = std::make_shared<UnauthorizedDisplayChanger>(display_changer);
    }

    if (session_authorizer->screencast_is_allowed(creds))
    {
        effective_screencast = screencast;
    }
    else
    {
        effective_screencast = std::make_shared<UnauthorizedScreencast>();
    }

    return make_mediator(
        shell,
        graphics_platform,
        changer,
        buffer_allocator,
        sm_report,
        sink,
        effective_screencast,
        connection_context);
}

std::shared_ptr<mf::ResourceCache> mf::DefaultIpcFactory::resource_cache()
{
    return cache;
}

std::shared_ptr<mf::detail::DisplayServer> mf::DefaultIpcFactory::make_mediator(
    std::shared_ptr<Shell> const& shell,
    std::shared_ptr<mg::Platform> const& graphics_platform,
    std::shared_ptr<DisplayChanger> const& changer,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<SessionMediatorReport> const& sm_report,
    std::shared_ptr<EventSink> const& sink,
    std::shared_ptr<Screencast> const& effective_screencast,
    ConnectionContext const& connection_context)
{
    return std::make_shared<SessionMediator>(
        shell,
        graphics_platform,
        changer,
        buffer_allocator->supported_pixel_formats(),
        sm_report,
        sink,
        resource_cache(),
        effective_screencast,
        connection_context);
}
