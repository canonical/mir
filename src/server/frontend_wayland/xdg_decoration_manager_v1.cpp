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

#include "xdg_decoration_manager_v1.h"

#include <mir/decoration_strategy.h>
#include <mir/log.h>
#include <mir/shell/surface_specification.h>

#include "client.h"
#include "protocol_error.h"
#include "weak.h"
#include "xdg_shell_stable.h"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>

namespace mir
{
namespace frontend
{
namespace
{
namespace mw = mir::wayland;

class ToplevelsWithDecorations
{
public:
    ToplevelsWithDecorations() = default;
    ToplevelsWithDecorations(ToplevelsWithDecorations const&) = delete;
    ToplevelsWithDecorations& operator=(ToplevelsWithDecorations const&) = delete;

    /// \return true if no duplicates existed before insertion, false otherwise.
    bool register_toplevel(XdgToplevelStable* toplevel)
    {
        auto [_, inserted] = toplevels_with_decorations.insert(toplevel);
        return inserted;
    }

    /// \return true if the toplevel was still registered, false otherwise.
    bool unregister_toplevel(XdgToplevelStable* toplevel)
    {
        return toplevels_with_decorations.erase(toplevel) > 0;
    }

private:
    std::unordered_set<XdgToplevelStable*> toplevels_with_decorations;
};

class XdgToplevelDecorationV1 : public mw::XdgToplevelDecorationV1
{
public:
    XdgToplevelDecorationV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::XdgToplevelDecorationV1Middleware> instance,
        uint32_t object_id,
        XdgToplevelStable* toplevel,
        std::shared_ptr<DecorationStrategy> strategy);

    void set_mode(uint32_t mode) override;
    void unset_mode() override;

private:
    static auto to_mode(DecorationStrategy::DecorationsType) -> uint32_t;
    auto to_decorations_type(uint32_t) -> DecorationStrategy::DecorationsType;
    void update_mode(uint32_t new_mode);

    XdgToplevelStable* toplevel;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
};

class XdgDecorationManagerV1 : public mw::XdgDecorationManagerV1
{
public:
    XdgDecorationManagerV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::XdgDecorationManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<DecorationStrategy> strategy);

private:
    auto get_toplevel_decoration(
        mw::Weak<mw::XdgToplevel> const& toplevel,
        rust::Box<mw::XdgToplevelDecorationV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::XdgToplevelDecorationV1> override;

    std::shared_ptr<ToplevelsWithDecorations> const toplevels_with_decorations;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
};
}
} // namespace frontend
} // namespace mir

namespace mf = mir::frontend;
namespace mw = mir::wayland;

auto mf::create_xdg_decoration_manager_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgDecorationManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<DecorationStrategy> strategy)
    -> std::shared_ptr<mw::XdgDecorationManagerV1>
{
    return std::make_shared<XdgDecorationManagerV1>(
        std::move(client), std::move(instance), object_id, std::move(strategy));
}

mf::XdgDecorationManagerV1::XdgDecorationManagerV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgDecorationManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<DecorationStrategy> strategy) :
    mw::XdgDecorationManagerV1{std::move(client), std::move(instance), object_id},
    toplevels_with_decorations{std::make_shared<ToplevelsWithDecorations>()},
    decoration_strategy{std::move(strategy)}
{
}

auto mf::XdgDecorationManagerV1::get_toplevel_decoration(
    mw::Weak<mw::XdgToplevel> const& toplevel,
    rust::Box<mw::XdgToplevelDecorationV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::XdgToplevelDecorationV1>
{
    using Error = mw::XdgToplevelDecorationV1::Error;

    auto* tl = XdgToplevelStable::from(toplevel);
    if (!tl)
    {
        throw std::runtime_error("Invalid toplevel pointer");
    }

    auto decoration = std::make_shared<XdgToplevelDecorationV1>(
        client, std::move(child_instance), child_object_id, tl, decoration_strategy);
    if (!toplevels_with_decorations->register_toplevel(tl))
    {
        throw mw::ProtocolError{
            object_id(), Error::already_constructed, "Decoration already constructed for this toplevel"};
    }

    decoration->add_destroy_listener(
        [toplevels_with_decorations = this->toplevels_with_decorations, tl]()
        {
            toplevels_with_decorations->unregister_toplevel(tl);
        });

    static_cast<mw::XdgToplevel*>(tl)->add_destroy_listener(
        [toplevels_with_decorations = this->toplevels_with_decorations, client = this->client, tl]()
        {
            // Under normal conditions, decorations should be destroyed before
            // toplevels. Causing `unregister_toplevel` to return false.
            //
            // If the attached decoration is not destroyed before its toplevel,
            // then its a protocol error. This can happen in two cases: A
            // protocol violation caused by the client, or another error
            // triggering wayland cleanup code which destroys wayland objects
            // with no guaranteed order.
            const auto orphaned_decoration = toplevels_with_decorations->unregister_toplevel(tl);
            if (!client->is_being_destroyed() && orphaned_decoration)
            {
                mir::log_warning("Toplevel destroyed before attached decoration!");
                // https://github.com/canonical/mir/issues/3452
            }
        });

    return decoration;
}

mf::XdgToplevelDecorationV1::XdgToplevelDecorationV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgToplevelDecorationV1Middleware> instance,
    uint32_t object_id,
    XdgToplevelStable* toplevel,
    std::shared_ptr<DecorationStrategy> strategy) :
    mw::XdgToplevelDecorationV1{std::move(client), std::move(instance), object_id},
    toplevel{toplevel},
    decoration_strategy{std::move(strategy)}
{
}

auto mf::XdgToplevelDecorationV1::to_mode(DecorationStrategy::DecorationsType type) -> uint32_t
{
    switch (type)
    {
    case DecorationStrategy::DecorationsType::ssd:
        return Mode::server_side;
    case DecorationStrategy::DecorationsType::csd:
        return Mode::client_side;
    }

    std::unreachable();
}

auto mf::XdgToplevelDecorationV1::to_decorations_type(uint32_t mode) -> DecorationStrategy::DecorationsType
{
    switch (mode)
    {
    case Mode::client_side:
        return DecorationStrategy::DecorationsType::csd;
    case Mode::server_side:
        return DecorationStrategy::DecorationsType::ssd;
    default:
    {
        mir::log_warning(
            "Client attempted to set invalid zxdg_toplevel_decoration_v1 mode (%u), defaulting to client side.", mode);

        return DecorationStrategy::DecorationsType::csd;
    }
    }
}

void mf::XdgToplevelDecorationV1::update_mode(uint32_t new_mode)
{
    auto spec = shell::SurfaceSpecification{};

    auto const new_type = decoration_strategy->request_style(to_decorations_type(new_mode));

    switch (new_type)
    {
    case DecorationStrategy::DecorationsType::ssd:
        spec.server_side_decorated = true;
        break;
    case DecorationStrategy::DecorationsType::csd:
        spec.server_side_decorated = false;
        break;
    }

    this->toplevel->apply_spec(spec);

    auto const strategy_new_mode = to_mode(new_type);
    send_configure_event(strategy_new_mode);
}

void mf::XdgToplevelDecorationV1::set_mode(uint32_t mode)
{
    update_mode(mode);
}

void mf::XdgToplevelDecorationV1::unset_mode()
{
    auto const protocol_mode = to_mode(decoration_strategy->default_style());
    update_mode(protocol_mode);
}
