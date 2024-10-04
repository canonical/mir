/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_SYSTEM_COMPOSITOR_WINDOW_MANAGER_H
#define MIRAL_SYSTEM_COMPOSITOR_WINDOW_MANAGER_H

#include "active_outputs.h"

#include <mir/shell/window_manager.h>
#include <mir/graphics/display_configuration.h>
#include <mir/observer_registrar.h>

#include <map>
#include <mutex>

namespace mir
{
namespace shell { class DisplayLayout; class FocusController; }
namespace scene { class SessionCoordinator; }
namespace graphics { class DisplayConfigurationObserver; }
}

namespace miral {

class DisplayConfigurationListeners;

/// A newer iteration of the SystemCompositorWindowManager which provides much of
/// the same functionality as its predecessor, but relies upon newer concepts
/// introduced by mirAL. This is the default WindowManager used by miral if no
/// other is specified.
///
/// (TODO: It will be a good idea to deprecate: src/include/server/mir/shell/system_compositor_window_manager.h
class SystemCompositorWindowManager: public mir::shell::WindowManager,
    private ActiveOutputsListener
{
public:
    SystemCompositorWindowManager(
        mir::shell::FocusController* focus_controller,
        std::shared_ptr<mir::shell::DisplayLayout> const& display_layout,
        std::shared_ptr<mir::scene::SessionCoordinator> const& session_coordinator,
        mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>& display_configuration_observers);

    ~SystemCompositorWindowManager();

/** @name Customization points
 * These are the likely events that a system compositor will care about
 *  @{ */
    /// Called when a session first connects (before any surfaces are ready)
    virtual void on_session_added(std::shared_ptr<mir::scene::Session> const& session) const;

    /// Called when a session disconnects
    virtual void on_session_removed(std::shared_ptr<mir::scene::Session> const& session) const;

    /// Called the first time each surface owned by the session posts its first buffer
    virtual void on_session_ready(std::shared_ptr<mir::scene::Session> const& session) const;
/** @} */

private:
    using OutputMap = std::map<std::weak_ptr<mir::scene::Surface>, mir::graphics::DisplayConfigurationOutputId, std::owner_less<std::weak_ptr<mir::scene::Surface>>>;
    using MirSurfaceCreator = std::function<std::shared_ptr<mir::scene::Surface>(std::shared_ptr<mir::scene::Session> const& session, mir::shell::SurfaceSpecification const& params)>;

    std::mutex mutable mutex;
    OutputMap output_map;
    mir::shell::FocusController* const focus_controller;
    std::shared_ptr<mir::shell::DisplayLayout> const display_layout;
    std::shared_ptr<mir::scene::SessionCoordinator> const session_coordinator;
    std::shared_ptr<DisplayConfigurationListeners> const display_config_monitor;

    void add_session(std::shared_ptr<mir::scene::Session> const& session) override;

    void remove_session(std::shared_ptr<mir::scene::Session> const& session) override;

    auto add_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::shell::SurfaceSpecification const& params,
        MirSurfaceCreator const& build) -> std::shared_ptr<mir::scene::Surface> override;

    void surface_ready(std::shared_ptr<mir::scene::Surface> const& surface) override;

    void modify_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        mir::shell::SurfaceSpecification const& modifications) override;

    void remove_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::weak_ptr<mir::scene::Surface> const& surface) override;

    void add_display(mir::geometry::Rectangle const& area) override;

    void remove_display(mir::geometry::Rectangle const& area) override;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    bool handle_touch_event(MirTouchEvent const* event) override;

    bool handle_pointer_event(MirPointerEvent const* event) override;

    void handle_raise_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        uint64_t timestamp) override;

    void handle_request_move(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        MirInputEvent const* event) override;

    void handle_request_resize(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        MirInputEvent const* event,
        MirResizeEdge edge) override;

    void advise_output_create(Output const& output) override;
    void advise_output_update(Output const& updated, Output const& original) override;
    void advise_output_delete(Output const& output) override;

    /// Called when an output change occurs. This will attempt to reposition
    /// a surface within an output.
    /// \param surface The surface in need of repositioning
    /// \param output_id The ID of the output in which we are positioning the surface
    void reposition_surface_in_output(
        std::shared_ptr<mir::scene::Surface> const& surface,
        mir::graphics::DisplayConfigurationOutputId output_id
    );
};
}


#endif //MIRAL_SYSTEM_COMPOSITOR_WINDOW_MANAGER_H
