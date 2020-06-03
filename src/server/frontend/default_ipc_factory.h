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

#ifndef MIR_FRONTEND_DEFAULT_IPC_FACTORY_H_
#define MIR_FRONTEND_DEFAULT_IPC_FACTORY_H_

#include "mir/extension_description.h"
#include "protobuf_ipc_factory.h"

namespace mir
{

class Executor;

namespace cookie
{
class Authority;
}
namespace graphics
{
class PlatformIpcOperations;
class GraphicBufferAllocator;
}
namespace input
{
class CursorImages;
}

namespace scene
{
class ApplicationNotRespondingDetector;
class CoordinateTranslator;
}

namespace frontend
{
class Shell;
class SessionMediatorObserver;
class DisplayChanger;
class Screencast;
class SessionAuthorizer;
class EventSinkFactory;
class InputConfigurationChanger;

class DefaultIpcFactory : public ProtobufIpcFactory
{
public:
    explicit DefaultIpcFactory(
        std::shared_ptr<Shell> const& shell,
        std::shared_ptr<SessionMediatorObserver> const& sm_observer,
        std::shared_ptr<graphics::PlatformIpcOperations> const& platform_ipc_operations,
        std::shared_ptr<DisplayChanger> const& display_changer,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<SessionAuthorizer> const& session_authorizer,
        std::shared_ptr<input::CursorImages> const& cursor_images,
        std::shared_ptr<scene::CoordinateTranslator> const& translator,
        std::shared_ptr<scene::ApplicationNotRespondingDetector> const& anr_detector,
        std::shared_ptr<cookie::Authority> const& cookie_authority,
        std::shared_ptr<InputConfigurationChanger> const& input_Changer,
        std::vector<mir::ExtensionDescription> const& extensions);

    std::shared_ptr<detail::DisplayServer> make_ipc_server(
        SessionCredentials const &creds,
        std::shared_ptr<EventSinkFactory> const& sink_factory,
        std::shared_ptr<MessageSender> const& message_sender,
        ConnectionContext const &connection_context) override;

    virtual std::shared_ptr<ResourceCache> resource_cache() override;

    virtual std::shared_ptr<detail::DisplayServer> make_mediator(
        std::shared_ptr<Shell> const& shell,
        std::shared_ptr<graphics::PlatformIpcOperations> const& platform_ipc_operations,
        std::shared_ptr<DisplayChanger> const& changer,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<SessionMediatorObserver> const& sm_observer,
        std::shared_ptr<EventSinkFactory> const& sink_factory,
        std::shared_ptr<MessageSender> const& message_sender,
        ConnectionContext const& connection_context,
        std::shared_ptr<input::CursorImages> const& cursor_images,
        std::shared_ptr<InputConfigurationChanger> const& input_changer);

private:
    std::shared_ptr<Shell> const shell;
    std::shared_ptr<Shell> const no_prompt_shell;
    std::shared_ptr<SessionMediatorObserver> const sm_observer;
    std::shared_ptr<ResourceCache> const cache;
    std::shared_ptr<graphics::PlatformIpcOperations> const platform_ipc_operations;
    std::shared_ptr<DisplayChanger> const display_changer;
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<SessionAuthorizer> const session_authorizer;
    std::shared_ptr<input::CursorImages> const cursor_images;
    std::shared_ptr<scene::CoordinateTranslator> const translator;
    std::shared_ptr<scene::ApplicationNotRespondingDetector> const anr_detector;
    std::shared_ptr<cookie::Authority> const cookie_authority;
    std::shared_ptr<InputConfigurationChanger> const input_changer;
    std::vector<mir::ExtensionDescription> const extensions;
    std::shared_ptr<mir::Executor> const execution_queue;
};
}
}

#endif /* MIR_FRONTEND_DEFAULT_IPC_FACTORY_H_ */
