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

#include "protocol_error.h"
#include "wl_surface.h"

#include <boost/throw_exception.hpp>

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>

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

    bool register_surface(WlSurface* surface)
    {
        auto [_, inserted] = surfaces_with_decorations.insert(surface);
        return inserted;
    }

    bool unregister_surface(WlSurface* surface)
    {
        return surfaces_with_decorations.erase(surface) > 0;
    }

private:
    std::unordered_set<WlSurface*> surfaces_with_decorations;
};

namespace
{
namespace mw = mir::wayland;

class ServerSurfaceDecoration : public mw::ServerDecoration
{
public:
    ServerSurfaceDecoration(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ServerDecorationMiddleware> instance,
        uint32_t object_id,
        WlSurface* surface,
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
    std::shared_ptr<DecorationStrategy> const decoration_strategy_;
    std::shared_ptr<SurfacesWithDecorations> bookkeeping_;
    uint32_t current_mode_;
};
}

ServerDecorationManager::ServerDecorationManager(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ServerDecorationManagerMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<DecorationStrategy> strategy)
    : mw::ServerDecorationManager{std::move(client), std::move(instance), object_id},
      surfaces_with_decorations{std::make_shared<SurfacesWithDecorations>()},
      decoration_strategy{std::move(strategy)}
{
    auto const default_mode =
        decoration_strategy->default_style() ==
                DecorationStrategy::DecorationsType::ssd
            ? mw::ServerDecorationManager::Mode::Server
            : mw::ServerDecorationManager::Mode::Client;

    send_default_mode_event(default_mode);
}

auto ServerDecorationManager::create(
    mw::Weak<mw::Surface> const& surface,
    rust::Box<mw::ServerDecorationMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::ServerDecoration>
{
    auto* const wl_surface = mw::Surface::from<WlSurface>(surface);
    if (!wl_surface)
    {
        throw mw::ProtocolError{object_id(), 0, "Invalid surface"};
    }

    if (!surfaces_with_decorations->register_surface(wl_surface))
    {
        throw mw::ProtocolError{
            object_id(), 0, "Decoration already constructed for this surface"};
    }

    auto decoration = std::make_shared<ServerSurfaceDecoration>(
        client,
        std::move(child_instance),
        child_object_id,
        wl_surface,
        decoration_strategy,
        surfaces_with_decorations);

    wl_surface->add_destroy_listener(
        [bookkeeping = surfaces_with_decorations, wl_surface]()
        {
            bookkeeping->unregister_surface(wl_surface);
        });

    return decoration;
}

ServerSurfaceDecoration::ServerSurfaceDecoration(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ServerDecorationMiddleware> instance,
    uint32_t object_id,
    WlSurface* surface,
    std::shared_ptr<DecorationStrategy> strategy,
    std::shared_ptr<SurfacesWithDecorations> bookkeeping)
    : mw::ServerDecoration{std::move(client), std::move(instance), object_id},
      surface_{surface},
      decoration_strategy_{std::move(strategy)},
      bookkeeping_{std::move(bookkeeping)},
      current_mode_{
          decoration_strategy_->default_style() ==
                  DecorationStrategy::DecorationsType::ssd
              ? mw::ServerDecoration::Mode::Server
              : mw::ServerDecoration::Mode::Client}
{
    shell::SurfaceSpecification spec;
    spec.server_side_decorated =
        current_mode_ == mw::ServerDecoration::Mode::Server;
    surface_->update_surface_spec(spec);
    send_mode_event(current_mode_);
}

ServerSurfaceDecoration::~ServerSurfaceDecoration()
{
    if (surface_)
    {
        bookkeeping_->unregister_surface(surface_);
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
        return mw::ServerDecoration::Mode::Server;

    case DecorationStrategy::DecorationsType::csd:
        return mw::ServerDecoration::Mode::Client;
    }

    std::unreachable();
}

auto ServerSurfaceDecoration::to_decorations_type(uint32_t mode) const
    -> DecorationStrategy::DecorationsType
{
    switch (mode)
    {
    case mw::ServerDecoration::Mode::Server:
        return DecorationStrategy::DecorationsType::ssd;

    case mw::ServerDecoration::Mode::Client:
    case mw::ServerDecoration::Mode::None:
        return DecorationStrategy::DecorationsType::csd;

    default:
        mir::log_warning(
            "Client requested invalid KDE decoration mode %u",
            mode);

        return DecorationStrategy::DecorationsType::csd;
    }
}

void ServerSurfaceDecoration::request_mode(uint32_t mode)
{
    auto const requested_type = to_decorations_type(mode);
    auto const requested_mode = to_mode(requested_type);

    shell::SurfaceSpecification spec;
    spec.server_side_decorated =
        requested_mode == mw::ServerDecoration::Mode::Server;

    surface_->update_surface_spec(spec);

    current_mode_ = requested_mode;
    send_mode_event(current_mode_);
}

} // namespace frontend
} // namespace mir
