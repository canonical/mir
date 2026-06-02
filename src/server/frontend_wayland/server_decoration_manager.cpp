/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "server_decoration_manager.h"

#include <mir/decoration_strategy.h>
#include <mir/log.h>
#include <mir/shell/surface_specification.h>
#include <mir/wayland/client.h>
#include <mir/wayland/protocol_error.h>

#include "wl_surface.h"

#include <boost/throw_exception.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <unistd.h>

namespace mir
{
namespace frontend
{

class SurfacesWithDecorations
{
public:
    SurfacesWithDecorations() = default;
    SurfacesWithDecorations(SurfacesWithDecorations const&) = delete;
    SurfacesWithDecorations& operator=(SurfacesWithDecorations const&) = delete;

    bool register_surface(wl_resource* surface)
    {
        auto [_, inserted] = surfaces_with_decorations.insert(surface);
        return inserted;
    }

    bool unregister_surface(wl_resource* surface)
    {
        return surfaces_with_decorations.erase(surface) > 0;
    }

private:
    std::unordered_set<wl_resource*> surfaces_with_decorations;
};

class ServerDecorationManager : public wayland::ServerDecorationManager
{
public:
    ServerDecorationManager(
        wl_resource* resource,
        std::shared_ptr<DecorationStrategy> strategy);

    class Global : public wayland::ServerDecorationManager::Global
    {
    public:
        Global(
            wl_display* display,
            std::shared_ptr<DecorationStrategy> strategy);

    private:
        std::shared_ptr<DecorationStrategy> const decoration_strategy;

        void bind(wl_resource* new_server_decoration_manager) override;
    };

private:
    void create(wl_resource* id, wl_resource* surface) override;

    std::shared_ptr<SurfacesWithDecorations> const surfaces_with_decorations;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
};

class ServerSurfaceDecoration : public wayland::ServerDecoration
{
public:
    ServerSurfaceDecoration(
        wl_resource* id,
        WlSurface* surface,
        wl_resource* surface_resource,
        std::shared_ptr<DecorationStrategy> strategy,
        std::shared_ptr<SurfacesWithDecorations> bookkeeping);

    ~ServerSurfaceDecoration() override;

    void request_mode(uint32_t mode) override;
    void release() override;

private:
    auto to_decorations_type(uint32_t mode) const
        -> DecorationStrategy::DecorationsType;

    static auto to_mode(DecorationStrategy::DecorationsType type)
        -> uint32_t;

    WlSurface* surface_;
    wl_resource* const surface_resource_;
    std::shared_ptr<DecorationStrategy> const decoration_strategy_;
    std::shared_ptr<SurfacesWithDecorations> bookkeeping_;
    uint32_t current_mode_;
};

auto create_server_decoration_manager(
    wl_display* display,
    std::shared_ptr<DecorationStrategy> strategy)
    -> std::shared_ptr<wayland::ServerDecorationManager::Global>
{
    return std::make_shared<ServerDecorationManager::Global>(
        display,
        std::move(strategy));
}

ServerDecorationManager::Global::Global(
    wl_display* display,
    std::shared_ptr<DecorationStrategy> strategy)
    : wayland::ServerDecorationManager::Global::Global{
          display,
          Version<1>()},
      decoration_strategy{std::move(strategy)}
{
}

void ServerDecorationManager::Global::bind(
    wl_resource* new_server_decoration_manager)
{
    new ServerDecorationManager{
        new_server_decoration_manager,
        decoration_strategy};
}

ServerDecorationManager::ServerDecorationManager(
    wl_resource* resource,
    std::shared_ptr<DecorationStrategy> strategy)
    : mir::wayland::ServerDecorationManager{
          resource,
          Version<1>()},
      surfaces_with_decorations{
          std::make_shared<SurfacesWithDecorations>()},
      decoration_strategy{std::move(strategy)}
{
    auto const default_mode =
        decoration_strategy->default_style() ==
                DecorationStrategy::DecorationsType::ssd
            ? wayland::ServerDecorationManager::Mode::Server
            : wayland::ServerDecorationManager::Mode::Client;

    send_default_mode_event(static_cast<uint32_t>(default_mode));
}

void ServerDecorationManager::create(
    wl_resource* id,
    wl_resource* surface)
{
    if (!id)
        return;

    if (!surface)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Invalid surface pointer"));
    }

    auto* wl_surface = WlSurface::from(surface);
    if (!wl_surface)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Invalid surface pointer"));
    }

    if (!surfaces_with_decorations->register_surface(surface))
    {
        BOOST_THROW_EXCEPTION(
            wayland::ProtocolError(
                resource,
                0,
                "Decoration already constructed for this surface"));
    }

    try
    {
        auto* decoration = new ServerSurfaceDecoration{
            id,
            wl_surface,
            surface,
            decoration_strategy,
            surfaces_with_decorations};

        decoration->add_destroy_listener(
            [bookkeeping = surfaces_with_decorations, surface]()
            {
                bookkeeping->unregister_surface(surface);
            });

        wl_surface->add_destroy_listener(
            [bookkeeping = surfaces_with_decorations, surface]()
            {
                bookkeeping->unregister_surface(surface);
            });
    }
    catch (std::bad_alloc const&)
    {
        surfaces_with_decorations->unregister_surface(surface);

        if (wl_resource_get_client(id))
        {
            wl_client_post_no_memory(
                wl_resource_get_client(id));
        }

        wl_resource_destroy(id);
    }
}

ServerSurfaceDecoration::ServerSurfaceDecoration(
    wl_resource* id,
    WlSurface* surface,
    wl_resource* surface_resource,
    std::shared_ptr<DecorationStrategy> strategy,
    std::shared_ptr<SurfacesWithDecorations> bookkeeping)
    : wayland::ServerDecoration{id, Version<1>()},
      surface_{surface},
      surface_resource_{surface_resource},
      decoration_strategy_{std::move(strategy)},
      bookkeeping_{std::move(bookkeeping)},
      current_mode_{
          decoration_strategy_->default_style() ==
                  DecorationStrategy::DecorationsType::ssd
              ? wayland::ServerDecoration::Mode::Server
              : wayland::ServerDecoration::Mode::Client}
{
    shell::SurfaceSpecification spec;
    spec.server_side_decorated =
        current_mode_ == wayland::ServerDecoration::Mode::Server;
    surface_->update_surface_spec(spec);
    send_mode_event(current_mode_);
}

ServerSurfaceDecoration::~ServerSurfaceDecoration()
{
    if (surface_resource_)
    {
        bookkeeping_->unregister_surface(surface_resource_);
    }
}

void ServerSurfaceDecoration::release()
{
    // No-op
}

auto ServerSurfaceDecoration::to_mode(
    DecorationStrategy::DecorationsType type)
    -> uint32_t
{
    switch (type)
    {
    case DecorationStrategy::DecorationsType::ssd:
        return wayland::ServerDecoration::Mode::Server;

    case DecorationStrategy::DecorationsType::csd:
        return wayland::ServerDecoration::Mode::Client;
    }

    std::unreachable();
}

auto ServerSurfaceDecoration::to_decorations_type(uint32_t mode) const
    -> DecorationStrategy::DecorationsType
{
    switch (mode)
    {
    case wayland::ServerDecoration::Mode::Server:
        return DecorationStrategy::DecorationsType::ssd;

    case wayland::ServerDecoration::Mode::Client:
    case wayland::ServerDecoration::Mode::None:
        return DecorationStrategy::DecorationsType::csd;

    default:
    {
        pid_t pid{};
        wl_client_get_credentials(
            client->raw_client(),
            &pid,
            nullptr,
            nullptr);

        mir::log_warning(
            "Client PID: %d requested invalid KDE decoration mode %u",
            pid,
            mode);

        return DecorationStrategy::DecorationsType::csd;
    }
    }
}

void ServerSurfaceDecoration::request_mode(uint32_t mode)
{
    auto const requested_type = to_decorations_type(mode);
    auto const requested_mode = to_mode(requested_type);

    shell::SurfaceSpecification spec;
    spec.server_side_decorated =
        requested_mode == wayland::ServerDecoration::Mode::Server;

    surface_->update_surface_spec(spec);

    current_mode_ = requested_mode;
    send_mode_event(current_mode_);
}

} // namespace frontend
} // namespace mir
