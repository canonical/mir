/*
 * Copyright © 2013-2019 Canonical Ltd.
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
 */

#include "mir/default_server_configuration.h"

#include "mir/graphics/display.h"
#include "mir/renderer/gl/context.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/input/scene.h"
#include "mir/scene/session.h"
#include "mir/scene/session_container.h"
#include "mir/shell/display_configuration_controller.h"
#include "mir/main_loop.h"

#include "broadcasting_session_event_sink.h"
#include "mediating_display_changer.h"
#include "session_manager.h"
#include "surface_allocator.h"
#include "surface_stack.h"
#include "prompt_session_manager_impl.h"
#include "timeout_application_not_responding_detector.h"
#include "basic_clipboard.h"
#include "basic_text_input_hub.h"
#include "basic_idle_hub.h"
#include "mir/options/default_configuration.h"
#include "mir/frontend/display_changer.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;

std::shared_ptr<mc::Scene>
mir::DefaultServerConfiguration::the_scene()
{
    return scene_surface_stack([this]()
                         { return std::make_shared<ms::SurfaceStack>(the_scene_report()); });
}

std::shared_ptr<mi::Scene> mir::DefaultServerConfiguration::the_input_scene()
{
    return scene_surface_stack([this]()
                             { return std::make_shared<ms::SurfaceStack>(the_scene_report()); });
}

auto mir::DefaultServerConfiguration::the_surface_factory()
-> std::shared_ptr<ms::SurfaceFactory>
{
    return surface_factory(
        [this]()
        {
            return std::make_shared<ms::SurfaceAllocator>(
                the_default_cursor_image(),
                the_scene_report());
        });
}

std::shared_ptr<msh::SurfaceStack>
mir::DefaultServerConfiguration::the_surface_stack()
{
    return surface_stack([this]()
        -> std::shared_ptr<msh::SurfaceStack>
             {
                 auto const wrapped = scene_surface_stack([this]()
                     { return std::make_shared<ms::SurfaceStack>(the_scene_report()); });

                 return wrap_surface_stack(wrapped);
             });
}

std::shared_ptr<mf::SurfaceStack>
mir::DefaultServerConfiguration::the_frontend_surface_stack()
{
    return scene_surface_stack([this]()
        {
            return std::make_shared<ms::SurfaceStack>(the_scene_report());
        });
}

auto mir::DefaultServerConfiguration::wrap_surface_stack(std::shared_ptr<shell::SurfaceStack> const& wrapped)
-> std::shared_ptr<shell::SurfaceStack>
{
    return wrapped;
}

std::shared_ptr<ms::BroadcastingSessionEventSink>
mir::DefaultServerConfiguration::the_broadcasting_session_event_sink()
{
    return broadcasting_session_event_sink(
        []
        {
            return std::make_shared<ms::BroadcastingSessionEventSink>();
        });
}

std::shared_ptr<ms::SessionEventSink>
mir::DefaultServerConfiguration::the_session_event_sink()
{
    return the_broadcasting_session_event_sink();
}

std::shared_ptr<ms::SessionEventHandlerRegister>
mir::DefaultServerConfiguration::the_session_event_handler_register()
{
    return the_broadcasting_session_event_sink();
}

std::shared_ptr<ms::SessionContainer>
mir::DefaultServerConfiguration::the_session_container()
{
    return session_container(
        []{ return std::make_shared<ms::SessionContainer>(); });
}

std::shared_ptr<ms::MediatingDisplayChanger>
mir::DefaultServerConfiguration::the_mediating_display_changer()
{
    return mediating_display_changer(
        [this]()
        {
            return std::make_shared<ms::MediatingDisplayChanger>(
                the_display(),
                the_compositor(),
                the_display_configuration_policy(),
                the_session_container(),
                the_session_event_handler_register(),
                the_server_action_queue(),
                the_display_configuration_observer(),
                the_main_loop());
        });

}

std::shared_ptr<mf::DisplayChanger>
mir::DefaultServerConfiguration::the_frontend_display_changer()
{
    return the_mediating_display_changer();
}

std::shared_ptr<mir::DisplayChanger>
mir::DefaultServerConfiguration::the_display_changer()
{
    return the_mediating_display_changer();
}

std::shared_ptr<ms::SessionCoordinator>
mir::DefaultServerConfiguration::the_session_coordinator()
{
    return session_coordinator(
        [this]()
        {
            return std::make_shared<ms::SessionManager>(
                the_surface_stack(),
                the_surface_factory(),
                the_buffer_stream_factory(),
                the_session_container(),
                the_session_event_sink(),
                the_session_listener(),
                the_display(),
                the_application_not_responding_detector(),
                the_buffer_allocator(),
                the_display_configuration_observer_registrar());
        });
}

std::shared_ptr<ms::PromptSessionManager>
mir::DefaultServerConfiguration::the_prompt_session_manager()
{
    return prompt_session_manager(
        [this]()
        {
            return std::make_shared<ms::PromptSessionManagerImpl>(
                the_session_container(),
                the_prompt_session_listener());
        });
}

auto mir::DefaultServerConfiguration::the_main_clipboard()
-> std::shared_ptr<ms::Clipboard>
{
    return clipboard(
        []()
        {
            return std::make_shared<ms::BasicClipboard>();
        });
}

auto mir::DefaultServerConfiguration::the_primary_selection_clipboard()
-> std::shared_ptr<ms::Clipboard>
{
    return clipboard(
        []()
        {
            return std::make_shared<ms::BasicClipboard>();
        });
}

auto mir::DefaultServerConfiguration::the_text_input_hub()
-> std::shared_ptr<scene::TextInputHub>
{
    return text_input_hub(
        []()
        {
            return std::make_shared<ms::BasicTextInputHub>();
        });
}

auto mir::DefaultServerConfiguration::the_idle_hub()
-> std::shared_ptr<scene::IdleHub>
{
    return idle_hub(
        [this]()
        {
            return std::make_shared<ms::BasicIdleHub>(the_clock(), *the_main_loop());
        });
}

auto mir::DefaultServerConfiguration::the_application_not_responding_detector()
-> std::shared_ptr<scene::ApplicationNotRespondingDetector>
{
    return application_not_responding_detector(
        [this]() -> std::shared_ptr<scene::ApplicationNotRespondingDetector>
        {
            using namespace std::literals::chrono_literals;
            return wrap_application_not_responding_detector(
                std::make_shared<ms::TimeoutApplicationNotRespondingDetector>(
                    *the_main_loop(), 1s));
        });
}

auto mir::DefaultServerConfiguration::wrap_application_not_responding_detector(
    std::shared_ptr<scene::ApplicationNotRespondingDetector> const& wrapped)
        -> std::shared_ptr<scene::ApplicationNotRespondingDetector>
{
    return wrapped;
}

std::shared_ptr<msh::DisplayConfigurationController>
mir::DefaultServerConfiguration::the_display_configuration_controller()
{
    return the_mediating_display_changer();
}
