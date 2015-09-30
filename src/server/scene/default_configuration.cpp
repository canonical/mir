/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"

#include "mir/main_loop.h"
#include "mir/graphics/display.h"
#include "mir/graphics/gl_context.h"
#include "mir/input/scene.h"
#include "mir/abnormal_exit.h"
#include "mir/scene/session.h"
#include "mir/shell/display_configuration_controller.h"

#include "broadcasting_session_event_sink.h"
#include "default_session_container.h"
#include "gl_pixel_buffer.h"
#include "global_event_sender.h"
#include "mediating_display_changer.h"
#include "session_container.h"
#include "session_manager.h"
#include "surface_allocator.h"
#include "surface_controller.h"
#include "surface_stack.h"
#include "threaded_snapshot_strategy.h"
#include "prompt_session_manager_impl.h"
#include "default_coordinate_translator.h"
#include "timeout_application_not_responding_detector.h"
#include "mir/options/program_option.h"
#include "mir/options/default_configuration.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;

std::shared_ptr<ms::SurfaceStackModel>
mir::DefaultServerConfiguration::the_surface_stack_model()
{
    return surface_stack([this]()
                         { return std::make_shared<ms::SurfaceStack>(the_scene_report()); });
}

std::shared_ptr<mc::Scene>
mir::DefaultServerConfiguration::the_scene()
{
    return surface_stack([this]()
                         { return std::make_shared<ms::SurfaceStack>(the_scene_report()); });
}

std::shared_ptr<mi::Scene> mir::DefaultServerConfiguration::the_input_scene()
{
    return surface_stack([this]()
                         { return std::make_shared<ms::SurfaceStack>(the_scene_report()); });
}

auto mir::DefaultServerConfiguration::the_surface_factory()
-> std::shared_ptr<ms::SurfaceFactory>
{
    return surface_factory(
        [this]()
        {
            return std::make_shared<ms::SurfaceAllocator>(
                the_input_channel_factory(),
                the_input_sender(),
                the_default_cursor_image(),
                the_scene_report());
        });
}

std::shared_ptr<ms::SurfaceCoordinator>
mir::DefaultServerConfiguration::the_surface_coordinator()
{
    return surface_coordinator(
        [this]()
        {
            return std::make_shared<ms::SurfaceController>(
                    the_surface_factory(),
                    the_surface_stack_model());
        });
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
        []{ return std::make_shared<ms::DefaultSessionContainer>(); });
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
                the_display_configuration_report());
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

std::shared_ptr<mf::EventSink>
mir::DefaultServerConfiguration::the_global_event_sink()
{
    return global_event_sink(
        [this]()
        {
            return std::make_shared<ms::GlobalEventSender>(the_session_container());
        });
}

std::shared_ptr<ms::SessionCoordinator>
mir::DefaultServerConfiguration::the_session_coordinator()
{
    return session_coordinator(
        [this]()
        {
            return std::make_shared<ms::SessionManager>(
                    the_surface_coordinator(),
                    the_surface_factory(),
                    the_buffer_stream_factory(),
                    the_session_container(),
                    the_snapshot_strategy(),
                    the_session_event_sink(),
                    the_session_listener(),
                    the_application_not_responding_detector());
        });
}

std::shared_ptr<ms::PixelBuffer>
mir::DefaultServerConfiguration::the_pixel_buffer()
{
    return pixel_buffer(
        [this]()
        {
            return std::make_shared<ms::GLPixelBuffer>(
                the_display()->create_gl_context());
        });
}

std::shared_ptr<ms::SnapshotStrategy>
mir::DefaultServerConfiguration::the_snapshot_strategy()
{
    return snapshot_strategy(
        [this]()
        {
            return std::make_shared<ms::ThreadedSnapshotStrategy>(
                the_pixel_buffer());
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

std::shared_ptr<ms::CoordinateTranslator>
mir::DefaultServerConfiguration::the_coordinate_translator()
{
    return coordinate_translator(
        [this]()
        {
            return std::make_shared<ms::DefaultCoordinateTranslator>();
        });
}

auto mir::DefaultServerConfiguration::the_application_not_responding_detector()
-> std::shared_ptr<scene::ApplicationNotRespondingDetector>
{
    return application_not_responding_detector(
        [this]() -> std::shared_ptr<scene::ApplicationNotRespondingDetector>
        {
            using namespace std::literals::chrono_literals;
            return std::make_shared<ms::TimeoutApplicationNotRespondingDetector>(
                *the_main_loop(), 1s);
        });
}

std::shared_ptr<msh::DisplayConfigurationController>
mir::DefaultServerConfiguration::the_display_configuration_controller()
{
    return the_mediating_display_changer();
}
