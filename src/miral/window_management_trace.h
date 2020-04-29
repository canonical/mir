/*
 * Copyright © 2016-2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_WINDOW_MANAGEMENT_TRACE_H
#define MIRAL_WINDOW_MANAGEMENT_TRACE_H

#include "window_manager_tools_implementation.h"

#include "miral/window_manager_tools.h"
#include "miral/window_management_options.h"
#include "miral/window_management_policy.h"

#include <atomic>

namespace miral
{
class WindowManagementTrace
    : public WindowManagementPolicy,
      public WindowManagementPolicy::ApplicationZoneAddendum,
      WindowManagerToolsImplementation
{
public:
    WindowManagementTrace(WindowManagerTools const& wrapped, WindowManagementPolicyBuilder const& builder);

private:
    virtual auto count_applications() const -> unsigned int override;

    virtual void for_each_application(std::function<void(ApplicationInfo&)> const& functor) override;

    virtual auto find_application(std::function<bool(ApplicationInfo const& info)> const& predicate)
    -> Application override;

    virtual auto info_for(std::weak_ptr<mir::scene::Session> const& session) const -> ApplicationInfo& override;

    virtual auto info_for(std::weak_ptr<mir::scene::Surface> const& surface) const -> WindowInfo& override;

    virtual auto info_for(Window const& window) const -> WindowInfo& override;

    virtual void ask_client_to_close(Window const& window) override;

    virtual auto active_window() const -> Window override;
    virtual auto select_active_window(Window const& hint) -> Window override;
    virtual auto window_at(mir::geometry::Point cursor) const -> Window override;
    virtual auto active_output() -> mir::geometry::Rectangle const override;
    virtual auto info_for_window_id(std::string const& id) const -> WindowInfo& override;
    virtual auto id_for_window(Window const& window) const -> std::string override;
    virtual void place_and_size_for_state(WindowSpecification& modifications, WindowInfo const& window_info) const override;

    virtual void drag_active_window(mir::geometry::Displacement movement) override;

    void drag_window(Window const& window, mir::geometry::Displacement& movement) override;

    virtual void focus_next_application() override;
    virtual void focus_prev_application() override;

    virtual void focus_next_within_application() override;
    virtual void focus_prev_within_application() override;

    virtual void raise_tree(Window const& root) override;
    virtual void start_drag_and_drop(WindowInfo& window_info, std::vector<uint8_t> const& handle) override;
    virtual void end_drag_and_drop() override;

    virtual void modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    virtual void invoke_under_lock(std::function<void()> const& callback) override;

    virtual auto place_new_window(
        ApplicationInfo const& app_info,
        WindowSpecification const& requested_specification) -> WindowSpecification override;
    virtual void handle_window_ready(WindowInfo& window_info) override;

    virtual void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    virtual void handle_raise_window(WindowInfo& window_info) override;

    virtual bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    virtual bool handle_touch_event(MirTouchEvent const* event) override;

    virtual bool handle_pointer_event(MirPointerEvent const* event) override;

    auto confirm_inherited_move(WindowInfo const& window_info, Displacement movement) -> Rectangle override;

    auto create_workspace() -> std::shared_ptr<Workspace> override;

    void add_tree_to_workspace(Window const& window, std::shared_ptr<Workspace> const& workspace) override;

    void remove_tree_from_workspace(Window const& window, std::shared_ptr<Workspace> const& workspace) override;

    void move_workspace_content_to_workspace(
        std::shared_ptr<Workspace> const& to_workspace,
        std::shared_ptr<Workspace> const& from_workspace) override;

    void for_each_workspace_containing(
        Window const& window,
        std::function<void(std::shared_ptr<Workspace> const& workspace)> const& callback) override;

    void for_each_window_in_workspace(
        std::shared_ptr<Workspace> const& workspace, std::function<void(Window const&)> const& callback) override;

    void handle_request_drag_and_drop(WindowInfo& window_info) override;

    void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) override;

    void handle_request_resize(WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override;

    void advise_adding_to_workspace(
        std::shared_ptr<Workspace> const& workspace, std::vector<Window> const& windows) override;

    void advise_removing_from_workspace(
        std::shared_ptr<Workspace> const& workspace, std::vector<Window> const& windows) override;

    auto confirm_placement_on_display(
        WindowInfo const& window_info,
        MirWindowState new_state,
        Rectangle const& new_placement) -> Rectangle override;

public:
    virtual void advise_begin() override;

    virtual void advise_end() override;

    virtual void advise_new_app(ApplicationInfo& application) override;

    virtual void advise_delete_app(ApplicationInfo const& application) override;

    virtual void advise_new_window(WindowInfo const& window_info) override;

    virtual void advise_focus_lost(WindowInfo const& info) override;

    virtual void advise_focus_gained(WindowInfo const& window_info) override;

    virtual void advise_state_change(WindowInfo const& window_info, MirWindowState state) override;

    virtual void advise_move_to(WindowInfo const& window_info, Point top_left) override;

    virtual void advise_resize(WindowInfo const& window_info, Size const& new_size) override;

    virtual void advise_delete_window(WindowInfo const& window_info) override;

    virtual void advise_raise(std::vector<Window> const& windows) override;

    void advise_output_create(Output const& output) override;

    void advise_output_update(Output const& updated, Output const& original) override;

    void advise_output_delete(Output const& output) override;

    void advise_application_zone_create(Zone const& application_zone) override;

    void advise_application_zone_update(Zone const& updated, Zone const& original) override;

    void advise_application_zone_delete(Zone const& application_zone) override;

private:
    WindowManagerTools wrapped;
    std::unique_ptr<miral::WindowManagementPolicy> const policy;
    miral::WindowManagementPolicy::ApplicationZoneAddendum* const policy_application_zone_addendum;
    std::atomic<unsigned> mutable trace_count;
    std::function<void()> log_input;
};
}

#endif //MIRAL_WINDOW_MANAGEMENT_TRACE_H
