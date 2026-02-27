/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_SHELL_WITH_SURFACE_REGISTRY_H_
#define MIR_FRONTEND_SHELL_WITH_SURFACE_REGISTRY_H_

#include <mir/shell/shell.h>
#include <mir/geometry/forward.h>
#include <memory>

namespace mir
{
namespace wayland
{
template <typename T> class Weak;
}
namespace frontend
{
class WlSurface;
class SurfaceRegistry;

/// Wrapper for Shell that ensures SurfaceRegistry calls are paired with Shell calls
class ShellWithSurfaceRegistry : public shell::Shell
{
public:
    ShellWithSurfaceRegistry(
        std::shared_ptr<shell::Shell> const& wrapped_shell,
        std::shared_ptr<SurfaceRegistry> const& surface_registry);

    // Shell interface
    auto open_session(
        pid_t client_pid,
        Fd socket_fd,
        std::string const& name) -> std::shared_ptr<scene::Session> override;

    void close_session(std::shared_ptr<scene::Session> const& session) override;

    std::shared_ptr<scene::PromptSession> start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params) override;

    void add_prompt_provider_for(
        std::shared_ptr<scene::PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session) override;

    void stop_prompt_session(std::shared_ptr<scene::PromptSession> const& prompt_session) override;

    /// Creates a surface. Note: The surface must be registered with add_surface_to_registry()
    /// after creation since the WlSurface is not available at this point.
    auto create_surface(
        std::shared_ptr<scene::Session> const& session,
        shell::SurfaceSpecification const& params,
        std::shared_ptr<scene::SurfaceObserver> const& observer,
        Executor* observer_executor) -> std::shared_ptr<scene::Surface> override;

    void surface_ready(std::shared_ptr<scene::Surface> const& surface) override;

    void modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        shell::SurfaceSpecification const& modifications) override;

    /// Destroys a surface and removes it from the SurfaceRegistry
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

    // FocusController interface
    void focus_next_session() override;
    void focus_prev_session() override;
    auto focused_session() const -> std::shared_ptr<scene::Session> override;
    auto focused_surface() const -> std::shared_ptr<scene::Surface> override;
    void set_focus_to(
        std::shared_ptr<scene::Session> const& focus_session,
        std::shared_ptr<scene::Surface> const& focus_surface) override;
    void set_popup_grab_tree(std::shared_ptr<scene::Surface> const& surface) override;
    auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> override;
    void raise(shell::SurfaceSet const& surfaces) override;
    void swap_z_order(shell::SurfaceSet const& first, shell::SurfaceSet const& second) override;
    void send_to_back(shell::SurfaceSet const& surfaces) override;
    auto is_above(std::weak_ptr<scene::Surface> const& a, std::weak_ptr<scene::Surface> const& b) const -> bool override;

    // EventFilter interface
    auto handle(MirEvent const& event) -> bool override;

    // DisplayListener interface
    void add_display(geometry::Rectangle const& area) override;
    void remove_display(geometry::Rectangle const& area) override;

    /// Adds a surface to the registry with its corresponding WlSurface
    void add_surface_to_registry(
        std::shared_ptr<scene::Surface> const& surface,
        wayland::Weak<WlSurface> const& wl_surface);

private:
    std::shared_ptr<shell::Shell> const wrapped_shell;
    std::shared_ptr<SurfaceRegistry> const registry;
};

}
}

#endif // MIR_FRONTEND_SHELL_WITH_SURFACE_REGISTRY_H_
