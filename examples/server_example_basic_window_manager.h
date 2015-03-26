/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_EXAMPLE_BASIC_WINDOW_MANAGER_H_
#define MIR_EXAMPLE_BASIC_WINDOW_MANAGER_H_

#include "mir/geometry/rectangles.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/abstract_shell.h"
#include "mir/shell/window_manager.h"

#include <map>
#include <mutex>

///\example server_example_basic_window_manager.h
/// A generic policy-based window manager implementation

namespace mir
{
namespace examples
{
using shell::SurfaceSet;

template<typename Info>
struct SurfaceTo
{
    using type = std::map<std::weak_ptr<scene::Surface>, Info, std::owner_less<std::weak_ptr<scene::Surface>>>;
};

template<typename Info>
struct SessionTo
{
    using type = std::map<std::weak_ptr<scene::Session>, Info, std::owner_less<std::weak_ptr<scene::Session>>>;
};

/// The interface through which the policy instructs the controller.
/// These functions assume that the BasicWindowManager data structures can be accessed freely.
/// I.e. should only be invoked by the policy handle_... methods (where any necessary locks are held).
// TODO extract commonality with FocusController (once that's separated from shell::FocusController)
template<typename SessionInfo, typename SurfaceInfo>
class BasicWindowManagerTools
{
public:
    virtual auto find_session(std::function<bool(SessionInfo const& info)> const& predicate)
    -> std::shared_ptr<scene::Session> = 0;

    virtual auto info_for(std::weak_ptr<scene::Session> const& session) const -> SessionInfo& = 0;

    virtual auto info_for(std::weak_ptr<scene::Surface> const& surface) const -> SurfaceInfo& = 0;

    virtual std::shared_ptr<scene::Session> focused_session() const = 0;

    virtual std::shared_ptr<scene::Surface> focused_surface() const = 0;

    virtual void focus_next() = 0;

    virtual void set_focus_to(
        std::shared_ptr<scene::Session> const& focus,
        std::shared_ptr<scene::Surface> const& surface) = 0;

    virtual auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> = 0;

    virtual void raise(SurfaceSet const& surfaces) = 0;

    virtual ~BasicWindowManagerTools() = default;
    BasicWindowManagerTools() = default;
    BasicWindowManagerTools(BasicWindowManagerTools const&) = delete;
    BasicWindowManagerTools& operator=(BasicWindowManagerTools const&) = delete;
};

/// A policy based window manager.
/// This takes care of the management of any meta implementation held for the sessions and surfaces.
///
/// \tparam WindowManagementPolicy the constructor must take a pointer to BasicWindowManagerTools<>
/// as its first parameter. (Any additional parameters can be forwarded by
/// BasicWindowManager::BasicWindowManager.)
/// In addition WindowManagementPolicy must implement the following methods:
/// - void handle_session_info_updated(SessionInfoMap& session_info, Rectangles const& displays);
/// - void handle_displays_updated(SessionInfoMap& session_info, Rectangles const& displays);
/// - auto handle_place_new_surface(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& request_parameters) -> ms::SurfaceCreationParameters;
/// - void handle_new_surface(std::shared_ptr<ms::Session> const& session, std::shared_ptr<ms::Surface> const& surface);
/// - void handle_delete_surface(std::shared_ptr<ms::Session> const& /*session*/, std::weak_ptr<ms::Surface> const& /*surface*/);
/// - int handle_set_state(std::shared_ptr<ms::Surface> const& surface, MirSurfaceState value);
/// - bool handle_key_event(MirKeyInputEvent const* event);
/// - bool handle_touch_event(MirTouchInputEvent const* event);
/// - bool handle_pointer_event(MirPointerInputEvent const* event);
///
/// \tparam SessionInfo must be default constructable.
///
/// \tparam SurfaceInfo must be constructable from (std::shared_ptr<ms::Session>, std::shared_ptr<ms::Surface>)
template<typename WindowManagementPolicy, typename SessionInfo, typename SurfaceInfo>
class BasicWindowManager : public shell::WindowManager,
    private BasicWindowManagerTools<SessionInfo, SurfaceInfo>
{
public:
    template <typename... PolicyArgs>
    BasicWindowManager(
        shell::FocusController* focus_controller,
        PolicyArgs&&... policy_args) :
        focus_controller(focus_controller),
        policy(this, std::forward<PolicyArgs>(policy_args)...)
    {
    }

private:
    void add_session(std::shared_ptr<scene::Session> const& session) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        session_info[session] = SessionInfo();
        policy.handle_session_info_updated(session_info, displays);
    }

    void remove_session(std::shared_ptr<scene::Session> const& session) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        session_info.erase(session);
        policy.handle_session_info_updated(session_info, displays);
    }

    frontend::SurfaceId add_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        scene::SurfaceCreationParameters const placed_params = policy.handle_place_new_surface(session, params);
        auto const result = build(session, placed_params);
        auto const surface = session->surface(result);
        surface_info.emplace(surface, SurfaceInfo{session, surface});
        policy.handle_new_surface(session, surface);
        for (auto& decoration : policy.generate_decorations_for(session, surface))
            surface_info.emplace(decoration, SurfaceInfo{session, decoration});
        return result;
    }

    void remove_surface(
        std::shared_ptr<scene::Session> const& session,
        std::weak_ptr<scene::Surface> const& surface) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        policy.handle_delete_surface(session, surface);

        surface_info.erase(surface);
    }

    void add_display(geometry::Rectangle const& area) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        displays.add(area);
        policy.handle_displays_updated(session_info, displays);
    }

    void remove_display(geometry::Rectangle const& area) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        displays.remove(area);
        policy.handle_displays_updated(session_info, displays);
    }

    bool handle_key_event(MirKeyInputEvent const* event) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        return policy.handle_key_event(event);
    }

    bool handle_touch_event(MirTouchInputEvent const* event) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        return policy.handle_touch_event(event);
    }

    bool handle_pointer_event(MirPointerInputEvent const* event) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        return policy.handle_pointer_event(event);
    }

    int set_surface_attribute(
        std::shared_ptr<scene::Session> const& /*session*/,
        std::shared_ptr<scene::Surface> const& surface,
        MirSurfaceAttrib attrib,
        int value) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        switch (attrib)
        {
        case mir_surface_attrib_state:
        {
            auto const state = policy.handle_set_state(surface, MirSurfaceState(value));
            return surface->configure(attrib, state);
        }
        default:
            return surface->configure(attrib, value);
        }
    }

    auto find_session(std::function<bool(SessionInfo const& info)> const& predicate)
    -> std::shared_ptr<scene::Session> override
    {
        for(auto& info : session_info)
        {
            if (predicate(info.second))
            {
                return info.first.lock();
            }
        }

        return std::shared_ptr<scene::Session>{};
    }

    auto info_for(std::weak_ptr<scene::Session> const& session) const -> SessionInfo& override
    {
        return const_cast<SessionInfo&>(session_info.at(session));
    }

    auto info_for(std::weak_ptr<scene::Surface> const& surface) const -> SurfaceInfo& override
    {
        return const_cast<SurfaceInfo&>(surface_info.at(surface));
    }

    std::shared_ptr<scene::Session> focused_session() const override
    {
        return focus_controller->focused_session();
    }

    std::shared_ptr<scene::Surface> focused_surface() const override
    {
        return focus_controller->focused_surface();
    }

    void focus_next() override
    {
        focus_controller->focus_next();
    }

    void set_focus_to(
        std::shared_ptr<scene::Session> const& focus,
        std::shared_ptr<scene::Surface> const& surface) override
    {
        focus_controller->set_focus_to(focus, surface);
    }

    auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> override
    {
        return focus_controller->surface_at(cursor);
    }

    void raise(SurfaceSet const& surfaces) override
    {
        focus_controller->raise(surfaces);
    }

    shell::FocusController* const focus_controller;
    WindowManagementPolicy policy;

    std::mutex mutex;
    typename SessionTo<SessionInfo>::type session_info;
    typename SurfaceTo<SurfaceInfo>::type surface_info;
    geometry::Rectangles displays;
};
}
}

#endif /* MIR_EXAMPLE_BASIC_WINDOW_MANAGER_H_ */
