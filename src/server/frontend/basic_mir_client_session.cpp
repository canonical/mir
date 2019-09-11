/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "mir/frontend/basic_mir_client_session.h"

#include "mir/shell/shell.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/graphics/display_configuration_observer.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;

class mf::BasicMirClientSession::DisplayConfigurationObserver
    : public mg::DisplayConfigurationObserver
{
public:
    DisplayConfigurationObserver(BasicMirClientSession* session)
        : session{session}
    {
    }

    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {
    }

    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {
    }

    void base_configuration_updated(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {
    }

    void session_configuration_applied(
        std::shared_ptr<scene::Session> const&,
        std::shared_ptr<mg::DisplayConfiguration> const&) override
    {
    }

    void session_configuration_removed(std::shared_ptr<scene::Session> const&) override
    {
    }

    void configuration_failed(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override
    {
    }

    void catastrophic_configuration_error(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override
    {
    }

    void session_should_send_display_configuration(
        std::shared_ptr<ms::Session> const& session,
        std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        if (session == this->session->session_)
            this->session->send_display_config(*config);
    }

private:
    BasicMirClientSession* const session;
};

mf::BasicMirClientSession::BasicMirClientSession(
    std::shared_ptr<scene::Session> session,
    std::shared_ptr<frontend::EventSink> const& event_sink,
    graphics::DisplayConfiguration const& /*initial_display_config*/,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar)
    : session_{session},
      event_sink{event_sink},
      display_config_registrar{display_config_registrar},
      display_config_observer{std::make_shared<DisplayConfigurationObserver>(this)}
{
    display_config_registrar->register_interest(display_config_observer);
}

mf::BasicMirClientSession::~BasicMirClientSession()
{
    if (auto const registrar = display_config_registrar.lock())
        registrar->unregister_interest(*display_config_observer);
}

auto mf::BasicMirClientSession::name() const -> std::string
{
    return session_->name();
}

auto mf::BasicMirClientSession::get_surface(SurfaceId surface) const -> std::shared_ptr<Surface>
{
    return session_->surface(surface);
}

auto mf::BasicMirClientSession::create_surface(
    std::shared_ptr<shell::Shell> const& shell,
    scene::SurfaceCreationParameters const& params,
    std::shared_ptr<EventSink> const& sink) -> SurfaceId
{
    auto surface = shell->create_surface(session_, params, sink);
    return session_->surface_id(surface);
}

void mf::BasicMirClientSession::destroy_surface(std::shared_ptr<shell::Shell> const& shell, SurfaceId surface)
{
    shell->destroy_surface(session_, session_->surface(surface));
}

auto mf::BasicMirClientSession::create_buffer_stream(graphics::BufferProperties const& props) -> BufferStreamId
{
    return session_->create_buffer_stream(props);
}

auto mf::BasicMirClientSession::get_buffer_stream(BufferStreamId stream) const -> std::shared_ptr<BufferStream>
{
    return session_->get_buffer_stream(stream);
}

void mf::BasicMirClientSession::destroy_buffer_stream(BufferStreamId stream)
{
    session_->destroy_buffer_stream(stream);
}

void mf::BasicMirClientSession::send_display_config(graphics::DisplayConfiguration const& /*info*/)
{
}
