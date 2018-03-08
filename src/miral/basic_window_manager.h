/*
 * Copyright © 2015-2017 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_ABSTRACTION_BASIC_WINDOW_MANAGER_H_
#define MIR_ABSTRACTION_BASIC_WINDOW_MANAGER_H_

#include "window_manager_tools_implementation.h"

#include "miral/window_management_policy.h"
#include "miral/window_info.h"
#include "miral/active_outputs.h"
#include "miral/application.h"
#include "miral/application_info.h"
#include "mru_window_list.h"

#include <mir/geometry/rectangles.h>
#include <mir/observer_registrar.h>
#include <mir/shell/abstract_shell.h>
#include <mir/shell/window_manager.h>
#include <mir/version.h>

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>

#include <map>
#include <mutex>

namespace mir
{
namespace shell { class DisplayLayout; class PersistentSurfaceStore; }
namespace graphics { class DisplayConfigurationObserver; }
}

namespace miral
{
class WorkspacePolicy;
class WindowManagementPolicyAddendum2;
class WindowManagementPolicyAddendum3;
class WindowManagementPolicyAddendum4;
class DisplayConfigurationListeners;

using mir::shell::SurfaceSet;
using WindowManagementPolicyBuilder =
    std::function<std::unique_ptr<miral::WindowManagementPolicy>(miral::WindowManagerTools const& tools)>;

/// A policy based window manager.
/// This takes care of the management of any meta implementation held for the sessions and windows.
class BasicWindowManager : public virtual mir::shell::WindowManager,
    protected WindowManagerToolsImplementation,
    private ActiveOutputsListener
{
public:
    BasicWindowManager(
        mir::shell::FocusController* focus_controller,
        std::shared_ptr<mir::shell::DisplayLayout> const& display_layout,
        std::shared_ptr<mir::shell::PersistentSurfaceStore> const& persistent_surface_store,
        mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>& display_configuration_observers,
        WindowManagementPolicyBuilder const& build);
    ~BasicWindowManager();

    void add_session(std::shared_ptr<mir::scene::Session> const& session) override;

    void remove_session(std::shared_ptr<mir::scene::Session> const& session) override;

    auto add_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::function<mir::frontend::SurfaceId(std::shared_ptr<mir::scene::Session> const& session, mir::scene::SurfaceCreationParameters const& params)> const& build)
    -> mir::frontend::SurfaceId override;

    void modify_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        mir::shell::SurfaceSpecification const& modifications) override;

    void remove_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::weak_ptr<mir::scene::Surface> const& surface) override;

    void add_display(mir::geometry::Rectangle const& area) override
    __attribute__((deprecated("Mir doesn't reliably call this: it is ignored. Use add_display_for_testing() instead")));

    void add_display_for_testing(mir::geometry::Rectangle const& area);

    void remove_display(mir::geometry::Rectangle const& area) override;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    bool handle_touch_event(MirTouchEvent const* event) override;

    bool handle_pointer_event(MirPointerEvent const* event) override;

    void handle_raise_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        uint64_t timestamp) override;

    void handle_request_drag_and_drop(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        uint64_t timestamp) override;

    void handle_request_move(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        uint64_t timestamp) override;

    void handle_request_resize(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        uint64_t timestamp,
        MirResizeEdge edge) override;

    int set_surface_attribute(
        std::shared_ptr<mir::scene::Session> const& /*application*/,
        std::shared_ptr<mir::scene::Surface> const& surface,
        MirWindowAttrib attrib,
        int value) override;

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

    auto count_applications() const -> unsigned int override;

    void for_each_application(std::function<void(ApplicationInfo& info)> const& functor) override;

    auto find_application(std::function<bool(ApplicationInfo const& info)> const& predicate)
    -> Application override;

    auto info_for(std::weak_ptr<mir::scene::Session> const& session) const -> ApplicationInfo& override;

    auto info_for(std::weak_ptr<mir::scene::Surface> const& surface) const -> WindowInfo& override;

    auto info_for(Window const& window) const -> WindowInfo& override;

    void ask_client_to_close(Window const& window) override;

    void force_close(Window const& window) override;

    auto active_window() const -> Window override;

    auto select_active_window(Window const& hint) -> Window override;

    void drag_active_window(mir::geometry::Displacement movement) override;

    void drag_window(Window const& window, Displacement& movement) override;

    void focus_next_application() override;

    void focus_next_within_application() override;
    void focus_prev_within_application() override;

    auto window_at(mir::geometry::Point cursor) const -> Window override;

    auto active_output() -> mir::geometry::Rectangle const override;

    void raise_tree(Window const& root) override;
    void start_drag_and_drop(WindowInfo& window_info, std::vector<uint8_t> const& handle) override;
    void end_drag_and_drop() override;

    void modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    auto info_for_window_id(std::string const& id) const -> WindowInfo& override;

    auto id_for_window(Window const& window) const -> std::string override;
    void place_and_size_for_state(WindowSpecification& modifications, WindowInfo const& window_info) const override;

    void invoke_under_lock(std::function<void()> const& callback) override;

private:
    using SurfaceInfoMap = std::map<std::weak_ptr<mir::scene::Surface>, WindowInfo, std::owner_less<std::weak_ptr<mir::scene::Surface>>>;
    using SessionInfoMap = std::map<std::weak_ptr<mir::scene::Session>, ApplicationInfo, std::owner_less<std::weak_ptr<mir::scene::Session>>>;

    mir::shell::FocusController* const focus_controller;
    std::shared_ptr<mir::shell::DisplayLayout> const display_layout;
    std::shared_ptr<mir::shell::PersistentSurfaceStore> const persistent_surface_store;

    // Workspaces may die without any sync with the BWM mutex
    struct DeadWorkspaces
    {
        std::mutex mutable dead_workspaces_mutex;
        std::vector<std::weak_ptr<Workspace>> workspaces;
    };

    std::shared_ptr<DeadWorkspaces> const dead_workspaces{std::make_shared<DeadWorkspaces>()};

    std::unique_ptr<WindowManagementPolicy> const policy;
    WindowManagementPolicyAddendum2* const policy2;
    WindowManagementPolicyAddendum3* const policy3;
    WindowManagementPolicyAddendum4* const policy4;

    std::mutex mutex;
    SessionInfoMap app_info;
    SurfaceInfoMap window_info;
    mir::geometry::Rectangles outputs;
    mir::geometry::Point cursor;
    uint64_t last_input_event_timestamp{0};
#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(0, 27, 0)
    MirEvent const* last_input_event{nullptr};
#endif
    miral::MRUWindowList mru_active_windows;
    std::set<Window> fullscreen_surfaces;
    std::set<Window> maximized_surfaces;

    friend class Workspace;
    using wwbimap_t = boost::bimap<
        boost::bimaps::multiset_of<std::weak_ptr<Workspace>, std::owner_less<std::weak_ptr<Workspace>>>,
        boost::bimaps::multiset_of<Window>>;

    wwbimap_t workspaces_to_windows;

    std::shared_ptr<DisplayConfigurationListeners> const display_config_monitor;

    struct Locker;

    void update_event_timestamp(MirKeyboardEvent const* kev);
    void update_event_timestamp(MirPointerEvent const* pev);
    void update_event_timestamp(MirTouchEvent const* tev);
    void update_event_timestamp(MirInputEvent const* iev);

    auto can_activate_window_for_session(miral::Application const& session) -> bool;
    auto can_activate_window_for_session_in_workspace(
        miral::Application const& session,
        std::vector<std::shared_ptr<Workspace>> const& workspaces) -> bool;

    auto place_new_surface(ApplicationInfo const& app_info, WindowSpecification parameters) -> WindowSpecification;
    auto place_relative(mir::geometry::Rectangle const& parent, miral::WindowSpecification const& parameters, Size size)
        -> mir::optional_value<Rectangle>;

    void move_tree(miral::WindowInfo& root, mir::geometry::Displacement movement);
    void erase(miral::WindowInfo const& info);
    void validate_modification_request(WindowSpecification const& modifications, WindowInfo const& window_info) const;
    void place_and_size(WindowInfo& root, Point const& new_pos, Size const& new_size);
    void set_state(miral::WindowInfo& window_info, MirWindowState value);
    auto fullscreen_rect_for(WindowInfo const& window_info) const -> Rectangle;
    void remove_window(Application const& application, miral::WindowInfo const& info);
    void refocus(Application const& application, Window const& parent,
                 std::vector<std::shared_ptr<Workspace>> const& workspaces_containing_window);
    auto workspaces_containing(Window const& window) const -> std::vector<std::shared_ptr<Workspace>>;

    void advise_output_create(Output const& output) override;
    void advise_output_update(Output const& updated, Output const& original) override;
    void advise_output_delete(Output const& output) override;
    void update_windows_for_outputs();
};
}

#endif /* MIR_ABSTRACTION_BASIC_WINDOW_MANAGER_H_ */
