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

#ifndef MIR_SHELL_ABSTRACT_SHELL_H_
#define MIR_SHELL_ABSTRACT_SHELL_H_

#include "mir/shell/shell.h"
#include "mir/shell/window_manager_builder.h"
#include "mir/scene/surface_observer.h"

#include <mutex>

namespace mir
{
namespace input
{
class Seat;
}
namespace scene
{
class SessionCoordinator;
class PromptSessionManager;
}
namespace shell
{
class ShellReport;
class WindowManager;
class InputTargeter;
class SurfaceStack;
namespace decoration
{
class Manager;
}

/// Minimal Shell implementation with none of the necessary window management logic
class AbstractShell : public virtual Shell, public virtual FocusController
{
public:
    AbstractShell(
        std::shared_ptr<InputTargeter> const& input_targeter,
        std::shared_ptr<SurfaceStack> const& surface_stack,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager,
        std::shared_ptr<ShellReport> const& report,
        WindowManagerBuilder const& wm_builder,
        std::shared_ptr<input::Seat> const& seat,
        std::shared_ptr<decoration::Manager> const& decoration_manager);

    ~AbstractShell() noexcept;

    auto open_session(
        pid_t client_pid,
        Fd socket_fd,
        std::string const& name) -> std::shared_ptr<scene::Session> override;

    void close_session(std::shared_ptr<scene::Session> const& session) override;

    auto create_surface(
        std::shared_ptr<scene::Session> const& session,
        wayland::Weak<frontend::WlSurface> const& wayland_surface,
        SurfaceSpecification const& params,
        std::shared_ptr<scene::SurfaceObserver> const& observer,
        Executor* observer_executor) -> std::shared_ptr<scene::Surface> override;

    void surface_ready(std::shared_ptr<scene::Surface> const& surface) override;

    void modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        SurfaceSpecification const& modifications) override;

    void destroy_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface) override;

    void raise_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) override;

    void request_move(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirInputEvent const* event) override;

    void request_resize(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirInputEvent const* event,
        MirResizeEdge edge) override;

    std::shared_ptr<scene::PromptSession> start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params) override;

    void add_prompt_provider_for(
        std::shared_ptr<scene::PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session) override;

    void stop_prompt_session(std::shared_ptr<scene::PromptSession> const& prompt_session) override;

/** @name these come from FocusController
 * Focus changes are notified to the derived class via the private setting_focus_to()
 * functions.
 * \note I think the FocusController interface is unnecessary as:
 *   1. the functions are only meaningful in the context of implementing a Shell
 *   2. the implementation of these functions is Shell behaviour
 * Simply providing them as part of AbstractShell is probably adequate.
 *  @{ */
    void focus_next_session() override;
    void focus_prev_session() override;

    std::shared_ptr<scene::Session> focused_session() const override;

    void set_popup_grab_tree(std::shared_ptr<scene::Surface> const& surface) override;
    void set_focus_to(
        std::shared_ptr<scene::Session> const& focus_session,
        std::shared_ptr<scene::Surface> const& focus_surface) override;

    // The surface with focus
    std::shared_ptr<scene::Surface> focused_surface() const override;

    auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> override;

    void raise(SurfaceSet const& surfaces) override;
    void swap_z_order(SurfaceSet const& first, SurfaceSet const& second) override;
    void send_to_back(SurfaceSet const& surfaces) override;
    auto is_above(std::weak_ptr<scene::Surface> const& a, std::weak_ptr<scene::Surface> const& b) const -> bool override;
/** @} */

    void add_display(geometry::Rectangle const& area) override;
    void remove_display(geometry::Rectangle const& area) override;

    bool handle(MirEvent const& event) override;

protected:
    std::shared_ptr<InputTargeter> const input_targeter;
    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<scene::SessionCoordinator> const session_coordinator;
    std::shared_ptr<scene::PromptSessionManager> const prompt_session_manager;
    std::shared_ptr<WindowManager> const window_manager;
    std::shared_ptr<input::Seat> const seat;

private:
    class SurfaceConfinementUpdater;

    std::shared_ptr<ShellReport> const report;

    std::mutex mutable focus_mutex;
    std::weak_ptr<scene::Surface> focus_surface;
    std::weak_ptr<scene::Session> focus_session;
    std::vector<std::weak_ptr<scene::Surface>> notified_active_surfaces;
    std::weak_ptr<scene::Surface> last_requested_focus_surface;
    std::weak_ptr<scene::Surface> notified_keyboard_focus_surface;
    std::weak_ptr<scene::Surface> popup_parent;
    std::vector<std::weak_ptr<scene::Surface>> grabbing_popups;
    std::shared_ptr<SurfaceConfinementUpdater> const surface_confinement_updater;

    void notify_active_surfaces(
        std::unique_lock<std::mutex> const&,
        std::shared_ptr<scene::Surface> const& new_keyboard_focus_surface,
        std::vector<std::shared_ptr<scene::Surface>> new_active_surfaces);

    void set_keyboard_focus_surface(
        std::unique_lock<std::mutex> const& lock,
        std::shared_ptr<scene::Surface> const& new_keyboard_focus_surface);

    void update_focus_locked(
        std::unique_lock<std::mutex> const& lock,
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface);

    void set_popup_parent(std::shared_ptr<scene::Surface> const& new_popup_parent);

    void add_grabbing_popup(std::shared_ptr<scene::Surface> const& popup);

    void update_confinement_for(std::shared_ptr<scene::Surface> const& surface) const;

    std::shared_ptr<decoration::Manager> decoration_manager;
};
}
}

#endif /* MIR_SHELL_ABSTRACT_SHELL_H_ */
