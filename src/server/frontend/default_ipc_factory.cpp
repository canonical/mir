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

#include "no_prompt_shell.h"
#include "session_mediator.h"
#include "unauthorized_display_changer.h"
#include "unauthorized_screencast.h"
#include "resource_cache.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/frontend/event_sink.h"
#include "mir/graphics/graphic_buffer_allocator.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace ms = mir::scene;

mf::DefaultIpcFactory::DefaultIpcFactory(
    std::shared_ptr<Shell> const& shell,
    std::shared_ptr<SessionMediatorReport> const& sm_report,
    std::shared_ptr<mg::PlatformIpcOperations> const& platform_ipc_operations,
    std::shared_ptr<DisplayChanger> const& display_changer,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<Screencast> const& screencast,
    std::shared_ptr<SessionAuthorizer> const& session_authorizer,
    std::shared_ptr<mi::CursorImages> const& cursor_images,
    std::shared_ptr<scene::CoordinateTranslator> const& translator,
    std::shared_ptr<scene::ApplicationNotRespondingDetector> const& anr_detector) :
    shell(shell),
    no_prompt_shell(std::make_shared<NoPromptShell>(shell)),
    sm_report(sm_report),
    cache(std::make_shared<ResourceCache>()),
    platform_ipc_operations(platform_ipc_operations),
    display_changer(display_changer),
    buffer_allocator(buffer_allocator),
    screencast(screencast),
    session_authorizer(session_authorizer),
    cursor_images(cursor_images),
    translator{translator},
    anr_detector{anr_detector}
{
}

std::shared_ptr<mf::detail::DisplayServer> mf::DefaultIpcFactory::make_ipc_server(
    SessionCredentials const &creds,
    std::shared_ptr<EventSinkFactory> const& sink_factory,
    std::shared_ptr<mf::MessageSender> const& message_sender,
    ConnectionContext const &connection_context)
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

    auto const allow_prompt_session =
        session_authorizer->prompt_session_is_allowed(creds);

    auto const effective_shell = allow_prompt_session ? shell : no_prompt_shell;

    return make_mediator(
        effective_shell,
        platform_ipc_operations,
        changer,
        buffer_allocator,
        sm_report,
        sink_factory,
        message_sender,
        effective_screencast,
        connection_context, cursor_images);
}

std::shared_ptr<mf::ResourceCache> mf::DefaultIpcFactory::resource_cache()
{
    return cache;
}

std::shared_ptr<mf::detail::DisplayServer> mf::DefaultIpcFactory::make_mediator(
    std::shared_ptr<Shell> const& shell,
    std::shared_ptr<mg::PlatformIpcOperations> const& platform_ipc_operations,
    std::shared_ptr<DisplayChanger> const& changer,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<SessionMediatorReport> const& sm_report,
    std::shared_ptr<mf::EventSinkFactory> const& sink_factory,
    std::shared_ptr<mf::MessageSender> const& message_sender,
    std::shared_ptr<Screencast> const& effective_screencast,
    ConnectionContext const& connection_context,
    std::shared_ptr<mi::CursorImages> const& cursor_images)
{
    return std::make_shared<SessionMediator>(
        shell,
        platform_ipc_operations,
        changer,
        buffer_allocator->supported_pixel_formats(),
        sm_report,
        sink_factory,
        message_sender,
        resource_cache(),
        effective_screencast,
        connection_context,
        cursor_images,
        translator,
        anr_detector);
}
