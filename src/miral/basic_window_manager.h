/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_ABSTRACTION_BASIC_WINDOW_MANAGER_H_
#define MIR_ABSTRACTION_BASIC_WINDOW_MANAGER_H_

#include "window_manager_tools_implementation.h"

#include "miral/window_management_policy.h"
#include "miral/window_info.h"
#include "active_outputs.h"
#include "miral/application.h"
#include "miral/application_info.h"
#include "miral/zone.h"
#include "miral/output.h"
#include "mru_window_list.h"

#include <mir/geometry/rectangles.h>
#include <mir/observer_registrar.h>
#include <mir/shell/abstract_shell.h>
#include <mir/shell/window_manager.h>

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <optional>
#include <functional>

#include <map>
#include <mutex>

namespace mir
{
namespace shell { class DisplayLayout; class PersistentSurfaceStore; }
namespace graphics { class DisplayConfigurationObserver; }
}

namespace miral
{
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
        mir::shell::SurfaceSpecification const& params,
        std::function<std::shared_ptr<mir::scene::Surface>(
            std::shared_ptr<mir::scene::Session> const& session,
            mir::shell::SurfaceSpecification const& params)> const& build)
    -> std::shared_ptr<mir::scene::Surface> override;

    void surface_ready(std::shared_ptr<mir::scene::Surface> const& surface) override;

    void modify_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        mir::shell::SurfaceSpecification const& modifications) override;

    void remove_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::weak_ptr<mir::scene::Surface> const& surface) override;

    [[deprecated("Mir doesn't reliably call this: it is ignored. Use add_display_for_testing() instead")]]
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
        ::MirResizeEdge edge) override;

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

    auto active_window() const -> Window override;

    auto select_active_window(Window const& hint) -> Window override;

    void drag_active_window(mir::geometry::Displacement movement) override;

    void drag_window(Window const& window, Displacement& movement) override;

    void focus_next_application() override;
    void focus_prev_application() override;

    void focus_next_within_application() override;
    void focus_prev_within_application() override;

    auto window_to_select_application(const Application) const -> std::optional<Window> override;

    auto window_at(mir::geometry::Point cursor) const -> Window override;

    auto active_output() -> mir::geometry::Rectangle const override;
    auto active_application_zone() -> Zone override;

    void raise_tree(Window const& root) override;
    void swap_tree_order(Window const& first, Window const& second) override;
    void send_tree_to_back(Window const& root) override;
    void modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    auto info_for_window_id(std::string const& id) const -> WindowInfo& override;

    auto id_for_window(Window const& window) const -> std::string override;
    void place_and_size_for_state(WindowSpecification& modifications, WindowInfo const& window_info) const override;

    void invoke_under_lock(std::function<void()> const& callback) override;

private:
    /// An area for windows to be placed in
    struct DisplayArea
    {
        DisplayArea(Output const& output)
            : area{output.extents()},
              application_zone{area},
              contained_outputs{{output}}
        {
        }

        DisplayArea(Rectangle const& area)
            : area{area},
              application_zone{area}
        {
        }

        /// Returns the bounding rectangle of the extents of all contained outputs
        auto bounding_rectangle_of_contained_outputs() const -> Rectangle;

        /// Returns if this area is currently being used (update_application_zones() will remove it otherwise)
        auto is_alive() const -> bool;

        Rectangle area; ///< The full area. If there is a single output, this is the same as the output's extents
        /// The subset of the area where normal applications are generally placed (excludes, for example, panels)
        Zone application_zone;
        /// The last zone given to the policy, or nullopt if the policy hasn't been notified of this area's creation yet
        std::optional<Zone> zone_policy_knows_about;
        /// Often a single output
        /// can be empty or (in the case of logical output groups) contain multiple outputs
        /// if all outputs are removed the next call to update_application_zones() will drop this DisplayArea
        std::vector<Output> contained_outputs;
        /// Only set if this display area represents a single output
        std::optional<int> output_id;
        /// Only set if this display area represents a logical group of multiple outputs
        std::optional<int> logical_output_group_id;
        std::set<Window> attached_windows; ///< Maximized/anchored/etc windows attached to this area
    };

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

    std::mutex mutex;
    SessionInfoMap app_info;
    SurfaceInfoMap window_info;
    mir::geometry::Rectangles outputs;
    mir::geometry::Point cursor;
    uint64_t last_input_event_timestamp{0};
    miral::MRUWindowList mru_active_windows;
    bool allow_active_window = true;
    std::set<Window> fullscreen_surfaces;
    /// Generally maps 1:1 with outputs, but this should not be assumed
    /// For example, if multiple outputs are part of a logical output group they will have one big display area
    std::vector<std::shared_ptr<DisplayArea>> display_areas;
    /// If output configuration has changed and application zones need to be updated
    bool application_zones_need_update{false};

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

    auto surface_known(std::weak_ptr<mir::scene::Surface> const& surface, std::string const& action) -> bool;

    auto can_activate_window_for_session(miral::Application const& session) -> bool;
    auto can_activate_window_for_session_in_workspace(
        miral::Application const& session,
        std::vector<std::shared_ptr<Workspace>> const& workspaces) -> bool;

    auto place_new_surface(WindowSpecification parameters) -> WindowSpecification;
    auto place_relative(mir::geometry::Rectangle const& parent, miral::WindowSpecification const& parameters, Size size)
        -> mir::optional_value<Rectangle>;

    void move_tree(miral::WindowInfo& root, mir::geometry::Displacement movement);
    void set_tree_depth_layer(miral::WindowInfo& root, MirDepthLayer new_layer);
    void erase(miral::WindowInfo const& info);
    void validate_modification_request(WindowSpecification const& modifications, WindowInfo const& window_info) const;
    void place_and_size(WindowInfo& root, Point const& new_pos, Size const& new_size);
    void place_attached_to_zone(
        WindowInfo& info,
        mir::geometry::Rectangle const& application_zone);
    void update_attached_and_fullscreen_sets(WindowInfo const& window_info);
    void set_state(miral::WindowInfo& window_info, MirWindowState value);
    void remove_window(Application const& application, miral::WindowInfo const& info);
    void refocus(Application const& application, Window const& parent,
                 std::vector<std::shared_ptr<Workspace>> const& workspaces_containing_window);
    auto workspaces_containing(Window const& window) const -> std::vector<std::shared_ptr<Workspace>>;
    auto active_display_area() const -> std::shared_ptr<DisplayArea>;
    auto display_area_for_output_id(int output_id) const -> std::shared_ptr<DisplayArea>; ///< returns null if not found
    auto display_area_for(WindowInfo const& info) const -> std::shared_ptr<DisplayArea>;
    auto display_area_for(Rectangle const& rect) const -> std::optional<std::shared_ptr<DisplayArea>>;
    /// Returns the application zone area after shrinking it for the exclusive zone if needed
    static auto apply_exclusive_rect_to_application_zone(
        mir::geometry::Rectangle const& original_zone,
        mir::geometry::Rectangle const& exclusive_rect_global_coords,
        MirPlacementGravity attached_edges) -> mir::geometry::Rectangle;

    /// Adds the given output to an existing display area, or creates a new display area for it
    void add_output_to_display_areas(Locker const&, Output const& output);
    /// Removes the given output from it's display area. A future call to
    /// update_application_zones_and_attached_windows() may be needed to remove the display area.
    void remove_output_from_display_areas(Locker const&, Output const& output);
    void advise_output_create(Output const& output) override;
    void advise_output_update(Output const& updated, Output const& original) override;
    void advise_output_delete(Output const& output) override;
    void advise_output_end() override;
    /// Updates the application zones of all display areas and moves attached windows as needed
    void update_application_zones_and_attached_windows();

    /// Iterates each descendent window (including current) of the provided WindowInfo
    void for_each_descendent_in(WindowInfo const& info, std::function<void(const Window&)> func);
    /// Gathers windows provided WindowInfo
    auto collect_windows(WindowInfo const& info) -> SurfaceSet;
};
}

#endif /* MIR_ABSTRACTION_BASIC_WINDOW_MANAGER_H_ */
