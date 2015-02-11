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

#include "server_example_window_management.h"

#include "mir/scene/session.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/abstract_shell.h"

#include <map>
#include <mutex>

///\example server_example_basic_window_manager.h
/// A generic policy-based window manager implementation

namespace mir
{
namespace examples
{
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
template<typename SessionInfo, typename SurfaceInfo>
class BasicWindowManagerTools
{
public:
    virtual auto find_session(std::function<bool(SessionInfo const& info)> const& predicate)
    -> std::shared_ptr<scene::Session> = 0;

    virtual auto info_for(std::weak_ptr<scene::Session> const& session) const -> SessionInfo& = 0;

    virtual auto info_for(std::weak_ptr<scene::Surface> const& surface) const -> SurfaceInfo& = 0;

    virtual std::weak_ptr<scene::Session> focussed_application() const = 0;

    virtual void focus_next() = 0;

    virtual void set_focus_to(std::shared_ptr<scene::Session> const& focus) = 0;

    virtual ~BasicWindowManagerTools() = default;
    BasicWindowManagerTools() = default;
    BasicWindowManagerTools(BasicWindowManagerTools const&) = delete;
    BasicWindowManagerTools& operator=(BasicWindowManagerTools const&) = delete;
};

class WindowManagerMetadataModel
{
public:
    virtual void add_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual void remove_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual frontend::SurfaceId add_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build) = 0;

    virtual void remove_surface(
        std::weak_ptr<scene::Surface> const& surface,
        std::shared_ptr<scene::Session> const& session) = 0;

    virtual void add_display(geometry::Rectangle const& area) = 0;

    virtual void remove_display(geometry::Rectangle const& area) = 0;

    virtual bool handle_key_event(MirKeyInputEvent const* event) = 0;

    virtual bool handle_touch_event(MirTouchInputEvent const* event) = 0;

    virtual bool handle_pointer_event(MirPointerInputEvent const* event) = 0;

    virtual int handle_set_state(std::shared_ptr<scene::Surface> const& surface, MirSurfaceState value) = 0;

    virtual ~WindowManagerMetadataModel() = default;
    WindowManagerMetadataModel() = default;
    WindowManagerMetadataModel(WindowManagerMetadataModel const&) = delete;
    WindowManagerMetadataModel& operator=(WindowManagerMetadataModel const&) = delete;
};

template<typename WindowManagementPolicy, typename SessionInfo, typename SurfaceInfo>
class WindowManagerMetadatabase :
    public WindowManagerMetadataModel,
    private BasicWindowManagerTools<SessionInfo, SurfaceInfo>
{
public:
    template <typename... PolicyArgs>
    WindowManagerMetadatabase(
        shell::FocusController* focus_controller,
        PolicyArgs... policy_args) :
        focus_controller(focus_controller),
        policy(this, policy_args...)
    {
    }

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
        policy.handle_new_surface(session, surface);
        surface_info.emplace(surface, SurfaceInfo{session, surface});
        return result;
    }

    void remove_surface(
        std::weak_ptr<scene::Surface> const& surface,
        std::shared_ptr<scene::Session> const& session) override
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

    int handle_set_state(std::shared_ptr<scene::Surface> const& surface, MirSurfaceState value) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        return policy.handle_set_state(surface, value);
    }

private:
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

    std::weak_ptr<scene::Session> focussed_application() const override
    {
        return focus_controller->focussed_application();
    }

    void focus_next() override
    {
        focus_controller->focus_next();
    }

    void set_focus_to(std::shared_ptr<scene::Session> const& focus) override
    {
        focus_controller->set_focus_to(focus);
    }

    shell::FocusController* const focus_controller;
    WindowManagementPolicy policy;

    std::mutex mutex;
    typename SessionTo<SessionInfo>::type session_info;
    typename SurfaceTo<SurfaceInfo>::type surface_info;
    geometry::Rectangles displays;
};


class GenericWindowManager : public virtual Shell,
    private shell::AbstractShell
{
public:
    GenericWindowManager(
        std::shared_ptr<shell::InputTargeter> const& input_targeter,
        std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager,
        std::shared_ptr<WindowManagerMetadataModel> const& metadatabase) :
        AbstractShell(input_targeter, surface_coordinator, session_coordinator, prompt_session_manager),
        metadatabase{metadatabase}
    {
    }

    std::shared_ptr<scene::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) override
    {
        auto const result = shell::AbstractShell::open_session(client_pid, name, sink);
        metadatabase->add_session(result);
        return result;
    }

    void close_session(std::shared_ptr<scene::Session> const& session) override
    {
        metadatabase->remove_session(session);
        shell::AbstractShell::close_session(session);
    }

    frontend::SurfaceId create_surface(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params) override
    {
        auto const build = [this](std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& placed_params)
            {
                return shell::AbstractShell::create_surface(session, placed_params);
            };

        return metadatabase->add_surface(session, params, build);
    }

    void destroy_surface(std::shared_ptr<scene::Session> const& session, frontend::SurfaceId surface) override
    {
        metadatabase->remove_surface(session->surface(surface), session);
        shell::AbstractShell::destroy_surface(session, surface);
    }

    bool handle(MirEvent const& event) override
    {
        if (mir_event_get_type(&event) != mir_event_type_input)
            return false;

        auto const input_event = mir_event_get_input_event(&event);

        switch (mir_input_event_get_type(input_event))
        {
        case mir_input_event_type_key:
            return metadatabase->handle_key_event(mir_input_event_get_key_input_event(input_event));

        case mir_input_event_type_touch:
            return metadatabase->handle_touch_event(mir_input_event_get_touch_input_event(input_event));

        case mir_input_event_type_pointer:
            return metadatabase->handle_pointer_event(mir_input_event_get_pointer_input_event(input_event));
        }

        return false;
    }

    int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirSurfaceAttrib attrib,
        int value) override
    {
        switch (attrib)
        {
        case mir_surface_attrib_state:
        {
            auto const state = metadatabase->handle_set_state(surface, MirSurfaceState(value));
            return shell::AbstractShell::set_surface_attribute(session, surface, attrib, state);
        }
        default:
            return shell::AbstractShell::set_surface_attribute(session, surface, attrib, value);
        }
    }

private:
    void add_display(geometry::Rectangle const& area) override
    {
        metadatabase->add_display(area);
    }

    void remove_display(geometry::Rectangle const& area) override
    {
        metadatabase->remove_display(area);
    }

    std::shared_ptr<WindowManagerMetadataModel> const metadatabase;
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
class BasicWindowManager : public GenericWindowManager
{
public:
    template <typename... PolicyArgs>
    BasicWindowManager(
        std::shared_ptr<shell::InputTargeter> const& input_targeter,
        std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager,
        PolicyArgs... policy_args) :
        GenericWindowManager(
            input_targeter,
            surface_coordinator,
            session_coordinator,
            prompt_session_manager,
            std::make_shared<WindowManagerMetadatabase<WindowManagementPolicy, SessionInfo, SurfaceInfo>>(this, policy_args...))
    {
    }
};
}
}

#endif /* MIR_EXAMPLE_BASIC_WINDOW_MANAGER_H_ */
