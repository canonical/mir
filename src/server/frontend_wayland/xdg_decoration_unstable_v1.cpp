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

#include <mir/decoration_strategy.h>
#include <mir/log.h>
#include <mir/shell/surface_specification.h>
#include <mir/wayland/client.h>
#include <mir/wayland/protocol_error.h>

#include "xdg-decoration-unstable-v1_wrapper.h"
#include "xdg_output_v1.h"
#include "xdg_shell_stable.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

namespace mir
{
namespace frontend
{
/// Tracks, per toplevel, whether a decoration object is currently attached, the mode negotiated but not
/// yet committed, and the mode the surface actually has. An entry outlives any single decoration object so
/// the committed mode can be retained across a destroy+recreate with no intervening commit (xdg-decoration v2).
class ToplevelsWithDecorations
{
public:
    ToplevelsWithDecorations() = default;
    ToplevelsWithDecorations(ToplevelsWithDecorations const&) = delete;
    ToplevelsWithDecorations& operator=(ToplevelsWithDecorations const&) = delete;

    /// \return true if no decoration was already live for this toplevel, false otherwise.
    bool register_toplevel(wl_resource* toplevel)
    {
        auto& state = toplevels[toplevel];
        if (state.live)
            return false;
        state.live = true;
        return true;
    }

    /// \return true if the toplevel still had a live decoration registered, false otherwise.
    bool unregister_toplevel(wl_resource* toplevel)
    {
        auto const it = toplevels.find(toplevel);
        if (it == toplevels.end() || !it->second.live)
            return false;
        it->second.live = false;
        return true;
    }

    /// The toplevel itself is gone; nothing about it is worth remembering any more.
    void forget_toplevel(wl_resource* toplevel)
    {
        toplevels.erase(toplevel);
    }

    /// The mode the surface had at its last commit, if it hasn't since been reset by a commit with no
    /// decoration attached.
    auto committed_mode(wl_resource* toplevel) const -> std::optional<DecorationStrategy::DecorationsType>
    {
        if(auto const it = toplevels.find(toplevel); it != toplevels.end())
            return it->second.committed;
        return std::nullopt;
    }

    /// A mode has been negotiated; like other surface state it only takes effect on the next commit.
    void set_pending_mode(wl_resource* toplevel, DecorationStrategy::DecorationsType mode)
    {
        toplevels[toplevel].pending = mode;
    }

    /// Called on every commit of a toplevel that has ever had a decoration.
    /// \return true if server-side decorations must be removed in this commit.
    bool surface_committed(wl_resource* toplevel)
    {
        auto const it = toplevels.find(toplevel);
        if (it == toplevels.end())
            return false;

        auto& state = it->second;
        if (state.live)
        {
            if (state.pending)
                state.committed = std::exchange(state.pending, std::nullopt);
            return false;
        }

        // A commit with no decoration attached: the mode is assumed to be client-side again
        auto const had_ssd = state.committed == DecorationStrategy::DecorationsType::ssd ||
                             state.pending == DecorationStrategy::DecorationsType::ssd;
        state.committed.reset();
        state.pending.reset();
        return had_ssd;
    }

private:
    struct State
    {
        bool live{false};
        std::optional<DecorationStrategy::DecorationsType> pending;
        std::optional<DecorationStrategy::DecorationsType> committed;
    };

    std::unordered_map<wl_resource*, State> toplevels;
};

class XdgDecorationManagerV1 : public wayland::XdgDecorationManagerV1
{
public:
    XdgDecorationManagerV1(wl_resource* resource, std::shared_ptr<DecorationStrategy> strategy);

    class Global : public wayland::XdgDecorationManagerV1::Global
    {
    public:
        Global(wl_display* display, std::shared_ptr<DecorationStrategy> strategy);

    private:
        std::shared_ptr<DecorationStrategy> const decoration_strategy;
        void bind(wl_resource* new_zxdg_decoration_manager_v1) override;
    };

private:
    void get_toplevel_decoration(wl_resource* id, wl_resource* toplevel) override;

    std::shared_ptr<ToplevelsWithDecorations> const toplevels_with_decorations;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
};

class XdgToplevelDecorationV1 : public wayland::XdgToplevelDecorationV1
{
public:
    XdgToplevelDecorationV1(
        wl_resource* id,
        mir::frontend::XdgToplevelStable* toplevel,
        wl_resource* toplevel_resource,
        std::shared_ptr<DecorationStrategy> strategy,
        std::shared_ptr<ToplevelsWithDecorations> toplevels_with_decorations);

    void set_mode(uint32_t mode) override;
    void unset_mode() override;

private:
    static auto to_mode(DecorationStrategy::DecorationsType) -> uint32_t;
    auto to_decorations_type(uint32_t) -> DecorationStrategy::DecorationsType;
    void update_mode(uint32_t new_mode);

    mir::frontend::XdgToplevelStable* toplevel;
    wl_resource* const toplevel_resource;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
    std::shared_ptr<ToplevelsWithDecorations> const toplevels_with_decorations;
};
} // namespace frontend
} // namespace mir

auto mir::frontend::create_xdg_decoration_unstable_v1(wl_display* display, std::shared_ptr<DecorationStrategy> strategy)
    -> std::shared_ptr<mir::wayland::XdgDecorationManagerV1::Global>
{
    return std::make_shared<XdgDecorationManagerV1::Global>(display, std::move(strategy));
}

mir::frontend::XdgDecorationManagerV1::Global::Global(
    wl_display* display, std::shared_ptr<DecorationStrategy> strategy) :
    wayland::XdgDecorationManagerV1::Global::Global{display, Version<2>{}},
    decoration_strategy{std::move(strategy)}
{
}

void mir::frontend::XdgDecorationManagerV1::Global::bind(wl_resource* new_zxdg_decoration_manager_v1)
{
    new XdgDecorationManagerV1{new_zxdg_decoration_manager_v1, std::move(decoration_strategy)};
}

mir::frontend::XdgDecorationManagerV1::XdgDecorationManagerV1(
    wl_resource* resource, std::shared_ptr<DecorationStrategy> strategy) :
    mir::wayland::XdgDecorationManagerV1{resource, Version<2>{}},
    toplevels_with_decorations{std::make_shared<ToplevelsWithDecorations>()},
    decoration_strategy{std::move(strategy)}
{
}

void mir::frontend::XdgDecorationManagerV1::get_toplevel_decoration(wl_resource* id, wl_resource* toplevel)
{
    using Error = mir::frontend::XdgToplevelDecorationV1::Error;

    auto* tl = mir::frontend::XdgToplevelStable::from(toplevel);
    if (!tl)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid toplevel pointer"));
    }

    auto decoration = new XdgToplevelDecorationV1{id, tl, toplevel, decoration_strategy, toplevels_with_decorations};
    if (!toplevels_with_decorations->register_toplevel(toplevel))
    {
        throw mir::wayland::ProtocolError{
            resource, Error::already_constructed, "Decoration already constructed for this toplevel"};
    }

    decoration->add_destroy_listener(
        [toplevels_with_decorations = this->toplevels_with_decorations, toplevel]()
        {
            toplevels_with_decorations->unregister_toplevel(toplevel);
        });

    // Runs during handle_commit(), before the staged spec is applied, so changes made here land in this commit.
    // The hook is owned by tl, so tl is always alive when it runs.
    tl->set_commit_hook(
        [toplevels_with_decorations = this->toplevels_with_decorations, toplevel, tl]()
        {
            if (toplevels_with_decorations->surface_committed(toplevel))
            {
                shell::SurfaceSpecification spec;
                spec.server_side_decorated = false;
                tl->apply_spec(spec);
            }
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
                /* throw mir::wayland::ProtocolError{ */
                /*     resource, Error::orphaned, "Toplevel destroyed before its attached decoration"}; */
            }
            toplevels_with_decorations->forget_toplevel(toplevel);
        });
}

mir::frontend::XdgToplevelDecorationV1::XdgToplevelDecorationV1(
    wl_resource* id,
    mir::frontend::XdgToplevelStable* toplevel,
    wl_resource* toplevel_resource,
    std::shared_ptr<DecorationStrategy> strategy,
    std::shared_ptr<ToplevelsWithDecorations> toplevels_with_decorations) :
    wayland::XdgToplevelDecorationV1{id, Version<2>{}},
    toplevel{toplevel},
    toplevel_resource{toplevel_resource},
    decoration_strategy{std::move(strategy)},
    toplevels_with_decorations{std::move(toplevels_with_decorations)}
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
        pid_t pid;
        wl_client_get_credentials(client->raw_client(), &pid, nullptr, nullptr); // null pointers are allowed

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
    toplevels_with_decorations->set_pending_mode(toplevel_resource, new_type);

    auto const strategy_new_mode = to_mode(new_type);
    send_configure_event(strategy_new_mode);
}

void mir::frontend::XdgToplevelDecorationV1::set_mode(uint32_t mode)
{
    update_mode(mode);
}

void mir::frontend::XdgToplevelDecorationV1::unset_mode()
{
    // Keep the mode the surface actually has (e.g. retained across a destroy+recreate with no intervening
    // commit, per xdg-decoration v2); otherwise fall back to the compositor's default.
    auto const default_type =
        toplevels_with_decorations->committed_mode(toplevel_resource).value_or(decoration_strategy->default_style());
    auto const protocol_mode = to_mode(default_type);
    update_mode(protocol_mode);
}
