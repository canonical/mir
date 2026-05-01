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

#include "xdg_decoration_unstable_v1.h"
#include "client.h"
#include "protocol_error.h"

#include <mir/decoration_strategy.h>
#include <mir/log.h>
#include <mir/shell/surface_specification.h>

#include "xdg_output_v1.h"
#include "xdg_shell_stable.h"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>

namespace std {
    template <>
    struct hash<mir::wayland_rs::Weak<mir::wayland_rs::XdgToplevelImpl>> {
        size_t operator()(const mir::wayland_rs::Weak<mir::wayland_rs::XdgToplevelImpl>& k) const {
            // Replace 'get_raw_ptr()' with whatever method accesses
            // the unique underlying value (like a pointer or ID).
            return std::hash<void*>()(&k.value());
        }
    };
}

namespace mir
{
namespace frontend
{
class ToplevelsWithDecorations
{
public:
    ToplevelsWithDecorations() = default;
    ToplevelsWithDecorations(ToplevelsWithDecorations const&) = delete;
    ToplevelsWithDecorations& operator=(ToplevelsWithDecorations const&) = delete;

    /// \return true if no duplicates existed before insertion, false otherwise.
    bool register_toplevel(wayland_rs::Weak<wayland_rs::XdgToplevelImpl> const& toplevel)
    {
        auto [_, inserted] = toplevels_with_decorations.insert(toplevel);
        return inserted;
    }

    /// \return true if the toplevel was still registered, false otherwise.
    bool unregister_toplevel(wayland_rs::Weak<wayland_rs::XdgToplevelImpl> const& toplevel)
    {
        return toplevels_with_decorations.erase(toplevel) > 0;
    }

private:
    std::unordered_set<wayland_rs::Weak<wayland_rs::XdgToplevelImpl>> toplevels_with_decorations;
};



class XdgToplevelDecorationV1 : public wayland_rs::ZxdgToplevelDecorationV1Impl
{
public:
    XdgToplevelDecorationV1(
        std::shared_ptr<wayland_rs::Client> const& client,
        XdgToplevelStable* toplevel,
        std::shared_ptr<DecorationStrategy> strategy);

    void set_mode(uint32_t mode) override;
    void unset_mode() override;

private:
    static auto to_mode(DecorationStrategy::DecorationsType) -> uint32_t;
    auto to_decorations_type(uint32_t) -> DecorationStrategy::DecorationsType;
    void update_mode(uint32_t new_mode);

    std::shared_ptr<wayland_rs::Client> client;
    XdgToplevelStable* toplevel;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
};
} // namespace frontend
} // namespace mir

mir::frontend::XdgDecorationManagerV1::XdgDecorationManagerV1(
    std::shared_ptr<wayland_rs::Client> const& client,
    std::shared_ptr<DecorationStrategy> strategy) :
    client{client},
    toplevels_with_decorations{std::make_shared<ToplevelsWithDecorations>()},
    decoration_strategy{std::move(strategy)}
{
}

auto mir::frontend::XdgDecorationManagerV1::get_toplevel_decoration(wayland_rs::Weak<wayland_rs::XdgToplevelImpl> const& toplevel)
    -> std::shared_ptr<wayland_rs::ZxdgToplevelDecorationV1Impl>
{
    using Error = mir::frontend::XdgToplevelDecorationV1::Error;

    auto* tl = mir::frontend::XdgToplevelStable::from(&toplevel.value());
    if (!tl)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid toplevel pointer"));
    }

    auto decoration = std::make_shared<XdgToplevelDecorationV1>(client, tl, decoration_strategy);
    if (!toplevels_with_decorations->register_toplevel(toplevel))
    {
        BOOST_THROW_EXCEPTION(mir::wayland_rs::ProtocolError(
            object_id(), Error::already_constructed, "Decoration already constructed for this toplevel"));
    }

    decoration->add_destroy_listener(
        [toplevels_with_decorations = this->toplevels_with_decorations, toplevel]()
        {
            toplevels_with_decorations->unregister_toplevel(toplevel);
        });

    tl->add_destroy_listener(
        [toplevels_with_decorations = this->toplevels_with_decorations, client = this->client, toplevel]()
        {
            // Under normal conditions, decorations should be destroyed before
            // toplevels. Causing `unregister_toplevel` to return false.
            //
            // If the attached decoration is not destroyed before its toplevel,
            // then its a protocol error. This can happen in two cases: A
            // protocol violation caused by the client, or another error
            // triggering wayland cleanup code which destroys wayland objects
            // with no guaranteed order.
            const auto orphaned_decoration = toplevels_with_decorations->unregister_toplevel(toplevel);
            if (!client->is_being_destroyed() && orphaned_decoration)
            {
                mir::log_warning("Toplevel destroyed before attached decoration!");
                // https://github.com/canonical/mir/issues/3452
                /* BOOST_THROW_EXCEPTION(mir::wayland::ProtocolError( */
                /*     resource, Error::orphaned, "Toplevel destroyed before its attached decoration")); */
            }
        });
    return decoration;
}

mir::frontend::XdgToplevelDecorationV1::XdgToplevelDecorationV1(
    std::shared_ptr<wayland_rs::Client> const& client,
    XdgToplevelStable* toplevel,
    std::shared_ptr<DecorationStrategy> strategy) :
    client{client},
    toplevel{toplevel},
    decoration_strategy{std::move(strategy)}
{
}

auto mir::frontend::XdgToplevelDecorationV1::to_mode(DecorationStrategy::DecorationsType type) -> uint32_t
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

auto mir::frontend::XdgToplevelDecorationV1::to_decorations_type(uint32_t mode) -> DecorationStrategy::DecorationsType
{
    switch (mode)
    {
    case Mode::client_side:
        return DecorationStrategy::DecorationsType::csd;
    case Mode::server_side:
        return DecorationStrategy::DecorationsType::ssd;
    default:
    {
        pid_t pid = client->raw_client()->pid();
        mir::log_warning("Client PID: %d, attempted to set invalid zxdg_toplevel_decoration_v1 mode (%d), defaulting to client side.", pid, mode);

        return DecorationStrategy::DecorationsType::csd;
    }
    }
}

void mir::frontend::XdgToplevelDecorationV1::update_mode(uint32_t new_mode)
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

void mir::frontend::XdgToplevelDecorationV1::set_mode(uint32_t mode)
{
    update_mode(mode);
}

void mir::frontend::XdgToplevelDecorationV1::unset_mode()
{
    auto const protocol_mode = to_mode(decoration_strategy->default_style());
    update_mode(protocol_mode);
}
