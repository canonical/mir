/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
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
namespace shell
{
class ShellReport;
class WindowManager;

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
        std::shared_ptr<input::Seat> const& seat);

    ~AbstractShell() noexcept;

    auto open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) -> std::shared_ptr<scene::Session> override;

    void close_session(std::shared_ptr<scene::Session> const& session) override;

    auto create_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::shared_ptr<scene::SurfaceObserver> const& observer) -> std::shared_ptr<scene::Surface> override;

    void modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        SurfaceSpecification const& modifications) override;

    void destroy_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface) override;

    int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirWindowAttrib attrib,
        int value) override;

    int get_surface_attribute(
        std::shared_ptr<scene::Surface> const& surface,
        MirWindowAttrib attrib) override;

    void raise_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) override;

    void request_drag_and_drop(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) override;

    void request_move(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) override;

    void request_resize(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp,
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

    // More useful than FocusController::set_focus_to()!
    void set_focus_to(
        std::shared_ptr<scene::Session> const& focus_session,
        std::shared_ptr<scene::Surface> const& focus_surface) override;

    // The surface with focus
    std::shared_ptr<scene::Surface> focused_surface() const override;

    auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> override;

    void raise(SurfaceSet const& surfaces) override;
/** @} */

    void add_display(geometry::Rectangle const& area) override;
    void remove_display(geometry::Rectangle const& area) override;

    bool handle(MirEvent const& event) override;

    void update_focused_surface_confined_region();

    void set_drag_and_drop_handle(std::vector<uint8_t> const& handle) override;
    void clear_drag_and_drop_handle() override;

protected:
    std::shared_ptr<InputTargeter> const input_targeter;
    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<scene::SessionCoordinator> const session_coordinator;
    std::shared_ptr<scene::PromptSessionManager> const prompt_session_manager;
    std::shared_ptr<WindowManager> const window_manager;
    std::shared_ptr<input::Seat> const seat;

private:
    std::shared_ptr<ShellReport> const report;

    std::mutex mutable focus_mutex;
    std::weak_ptr<scene::Surface> focus_surface;
    std::weak_ptr<scene::Session> focus_session;
    std::weak_ptr<scene::Surface> notified_focus_surface;
    std::shared_ptr<scene::SurfaceObserver> focus_surface_observer;

    void notify_focus_locked(
        std::unique_lock<std::mutex> const& lock,
        std::shared_ptr<scene::Session> const& focus_session,
        std::shared_ptr<scene::Surface> const& focus_surface);

    void update_focus_locked(
        std::unique_lock<std::mutex> const& lock,
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface);
};
}
}

#endif /* MIR_SHELL_ABSTRACT_SHELL_H_ */
