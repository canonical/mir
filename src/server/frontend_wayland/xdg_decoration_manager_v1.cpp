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
#include "wayland_rs/src/ffi.rs.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

namespace mir
{
namespace frontend
{
namespace mw = mir::wayland;

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
    bool register_toplevel(XdgToplevelStable* toplevel)
    {
        auto& state = toplevels[toplevel];
        if (state.live)
            return false;
        state.live = true;
        return true;
    }

    /// \return true if the toplevel still had a live decoration registered, false otherwise.
    bool unregister_toplevel(XdgToplevelStable* toplevel)
    {
        auto const it = toplevels.find(toplevel);
        if (it == toplevels.end() || !it->second.live)
            return false;
        it->second.live = false;
        return true;
    }

    /// The toplevel itself is gone; nothing about it is worth remembering any more.
    void forget_toplevel(XdgToplevelStable* toplevel)
    {
        toplevels.erase(toplevel);
    }

    /// The mode the surface had at its last commit, if it hasn't since been reset by a commit with no
    /// decoration attached.
    auto committed_mode(XdgToplevelStable* toplevel) const -> std::optional<DecorationStrategy::DecorationsType>
    {
        if (auto const it = toplevels.find(toplevel); it != toplevels.end())
            return it->second.committed;
        return std::nullopt;
    }

    /// A mode has been negotiated; like other surface state it only takes effect on the next commit.
    void set_pending_mode(XdgToplevelStable* toplevel, DecorationStrategy::DecorationsType mode)
    {
        toplevels[toplevel].pending = mode;
    }

    /// Called on every commit of a toplevel that has ever had a decoration.
    /// \return the mode the surface must take in this commit, or nullopt if it is unchanged.
    auto surface_committed(XdgToplevelStable* toplevel) -> std::optional<DecorationStrategy::DecorationsType>
    {
        auto const it = toplevels.find(toplevel);
        if (it == toplevels.end())
            return std::nullopt;

        auto& state = it->second;
        if (state.live)
        {
            if (!state.pending)
                return std::nullopt;
            state.committed = std::exchange(state.pending, std::nullopt);
            return state.committed;
        }

        // A commit with no decoration attached: the mode is assumed to be client-side again
        auto const had_ssd = state.committed == DecorationStrategy::DecorationsType::ssd;
        state.committed.reset();
        state.pending.reset();
        if (had_ssd)
            return DecorationStrategy::DecorationsType::csd;
        return std::nullopt;
    }

private:
    struct State
    {
        bool live{false};
        std::optional<DecorationStrategy::DecorationsType> pending;
        std::optional<DecorationStrategy::DecorationsType> committed;
    };

    std::unordered_map<XdgToplevelStable*, State> toplevels;
};

namespace
{
class XdgToplevelDecorationV1 : public mw::XdgToplevelDecorationV1
{
public:
    XdgToplevelDecorationV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::XdgToplevelDecorationV1Middleware> instance,
        uint32_t object_id,
        XdgToplevelStable* toplevel,
        std::shared_ptr<DecorationStrategy> strategy,
        std::shared_ptr<ToplevelsWithDecorations> toplevels_with_decorations);

    void set_mode(uint32_t mode) override;
    void unset_mode() override;

    /// Whether a configure event has been sent for this decoration yet
    auto is_configured() const -> bool { return configured; }

private:
    static auto to_mode(DecorationStrategy::DecorationsType) -> uint32_t;
    auto to_decorations_type(uint32_t) -> DecorationStrategy::DecorationsType;
    void update_mode(uint32_t new_mode);

    XdgToplevelStable* toplevel;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
    bool configured{false};
    std::shared_ptr<ToplevelsWithDecorations> const toplevels_with_decorations;
};

class XdgDecorationManagerV1 : public mw::XdgDecorationManagerV1
{
public:
    XdgDecorationManagerV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::XdgDecorationManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<DecorationStrategy> strategy,
        std::shared_ptr<ToplevelsWithDecorations> toplevels_with_decorations);

private:
    auto get_toplevel_decoration(
        mw::Weak<mw::XdgToplevel> const& toplevel,
        rust::Box<mw::XdgToplevelDecorationV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::XdgToplevelDecorationV1> override;

    /// Shared by every manager binding, so a client can't bypass already_constructed or lose a retained
    /// mode by binding the manager again.
    std::shared_ptr<ToplevelsWithDecorations> const toplevels_with_decorations;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
};
}
} // namespace frontend
} // namespace mir

namespace mf = mir::frontend;
namespace mw = mir::wayland;

auto mf::make_toplevels_with_decorations() -> std::shared_ptr<ToplevelsWithDecorations>
{
    return std::make_shared<ToplevelsWithDecorations>();
}

auto mf::create_xdg_decoration_manager_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgDecorationManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<DecorationStrategy> strategy,
    std::shared_ptr<ToplevelsWithDecorations> toplevels_with_decorations)
    -> std::shared_ptr<mw::XdgDecorationManagerV1>
{
    return std::make_shared<XdgDecorationManagerV1>(
        std::move(client), std::move(instance), object_id, std::move(strategy), std::move(toplevels_with_decorations));
}

mf::XdgDecorationManagerV1::XdgDecorationManagerV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgDecorationManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<DecorationStrategy> strategy,
    std::shared_ptr<ToplevelsWithDecorations> toplevels_with_decorations) :
    mw::XdgDecorationManagerV1{std::move(client), std::move(instance), object_id},
    toplevels_with_decorations{std::move(toplevels_with_decorations)},
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
        client, std::move(child_instance), child_object_id, tl, decoration_strategy, toplevels_with_decorations);

    auto const version = get_box()->version();
    if (version < 2 && tl->has_buffer())
    {
        throw mw::ProtocolError{
            decoration->object_id(),
            Error::unconfigured_buffer,
            "Decoration created for a toplevel that already has a buffer"};
    }

    if (!toplevels_with_decorations->register_toplevel(tl))
    {
        throw mw::ProtocolError{
            object_id(), Error::already_constructed, "Decoration already constructed for this toplevel"};
    }

    auto const initial_mode =
        toplevels_with_decorations->committed_mode(tl).value_or(DecorationStrategy::DecorationsType::csd);
    toplevels_with_decorations->set_pending_mode(tl, initial_mode);

    decoration->add_destroy_listener(
        [toplevels_with_decorations = this->toplevels_with_decorations, tl]()
        {
            toplevels_with_decorations->unregister_toplevel(tl);
        });

    if (version < 2)
    {
        tl->set_attach_hook(
            [weak_decoration = mw::make_weak(decoration.get())]() -> std::optional<mw::ProtocolError>
            {
                if (weak_decoration && !weak_decoration.value().is_configured())
                {
                    return mw::ProtocolError{
                        weak_decoration.value().object_id(),
                        Error::unconfigured_buffer,
                        "Buffer attached before the decoration's first configure"};
                }
                return std::nullopt;
            });
    }

    // Runs during handle_commit(), before the staged spec is applied, so changes made here land in this commit.
    // The hook is owned by tl, so tl is always alive when it runs.
    tl->set_commit_hook(
        [toplevels_with_decorations = this->toplevels_with_decorations, tl]()
        {
            if (auto const mode = toplevels_with_decorations->surface_committed(tl))
            {
                shell::SurfaceSpecification spec;
                spec.server_side_decorated = *mode == DecorationStrategy::DecorationsType::ssd;
                tl->apply_spec(spec);
            }
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
            toplevels_with_decorations->forget_toplevel(tl);
        });

    return decoration;
}

mf::XdgToplevelDecorationV1::XdgToplevelDecorationV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgToplevelDecorationV1Middleware> instance,
    uint32_t object_id,
    XdgToplevelStable* toplevel,
    std::shared_ptr<DecorationStrategy> strategy,
    std::shared_ptr<ToplevelsWithDecorations> toplevels_with_decorations) :
    mw::XdgToplevelDecorationV1{std::move(client), std::move(instance), object_id},
    toplevel{toplevel},
    decoration_strategy{std::move(strategy)},
    toplevels_with_decorations{std::move(toplevels_with_decorations)}
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
        throw mw::ProtocolError{
            object_id(), Error::invalid_mode, "Invalid decoration mode %u", mode};
    }
}

void mf::XdgToplevelDecorationV1::update_mode(uint32_t new_mode)
{
    auto const new_type = decoration_strategy->request_style(to_decorations_type(new_mode));

    toplevels_with_decorations->set_pending_mode(toplevel, new_type);

    auto const strategy_new_mode = to_mode(new_type);
    send_configure_event(strategy_new_mode);
    toplevel->send_toplevel_configure();
    configured = true;
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
