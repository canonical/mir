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

#ifndef MIRAL_WINDOW_SPECIFICATION_H
#define MIRAL_WINDOW_SPECIFICATION_H

#include <mir_toolkit/common.h>

#include <mir/geometry/displacement.h>
#include <mir/geometry/rectangles.h>
#include <mir/optional_value.h>
#include <mir/int_wrapper.h>
#include <mir/flags.h>

#include <memory>

namespace mir
{
namespace scene { class Surface; }
namespace shell { struct SurfaceSpecification; }
}

namespace miral
{
using namespace mir::geometry;
namespace detail { struct SessionsBufferStreamIdTag; }
typedef mir::IntWrapper<detail::SessionsBufferStreamIdTag> BufferStreamId;

/// The window specification class describes a request of changes to be made
/// on a #miral::Window.
///
/// Instances of this class may originate either from the client or the compositor
/// itself. Typically, changes will be made from #miral::WindowManagementPolicy
/// and sent to #miral::WindowManagerTools::modify_window to enact the changes.
///
/// The class itself consists of optional values. If a value is set, then a change
/// on that value will occur. If the value is unset, then no change will happen.
///
/// \sa miral::WindowManagementPolicy - handles specifications from both the client and compositor
/// \sa miral::WindowManagerTools::modify_window - the method used to modify a window using a specification
class WindowSpecification
{
public:
    /// Describes the input reception mode.
    ///
    /// Used by #WindowSpecification::input_mode.
    enum class InputReceptionMode
    {
        normal,
        receives_all_input
    };

    /// Describes the aspect ratio.
    ///
    /// Used by #WindowSpecification::min_aspect and #WindowSpecification::max_aspect.
    struct AspectRatio { unsigned width; unsigned height; };

    /// Construct a new window specification.
    WindowSpecification();

    /// Construct a copy of the given speficiation.
    ///
    /// \param that to copy
    WindowSpecification(WindowSpecification const& that);
    auto operator=(WindowSpecification const& that) -> WindowSpecification&;

    /// Construct a window specification from a surface specification.
    ///
    /// For internal use only.
    ///
    /// \param spec the surface specification
    WindowSpecification(mir::shell::SurfaceSpecification const& spec);

    ~WindowSpecification();

    /// The top left corner of the window.
    ///
    /// \returns a const reference to the top left point
    auto top_left() const -> mir::optional_value<Point> const&;

    /// The size of the window frame, including any decorations.
    ///
    /// This value will be adjusted based on #min_width(), #WindowInfo::max_width(),
    /// #WindowInfo::min_height(), #WindowInfo::max_height(), #WindowInfo::min_aspect(),
    /// #WindowInfo::max_aspect(), #WindowInfo::width_inc() and WindowInfo::height_inc().
    /// Set these properties to their default values if they should be ignored.
    /// Note that the position of the window may also be adjusted if the new size violates
    /// the size constraints.
    ///
    /// This value is **not** guaranteed to be honored by the client.
    ///
    /// \returns a const reference to the size
    auto size() const -> mir::optional_value<Size> const&;

    /// The name of the window.
    ///
    /// \returns a const reference to the name
    auto name() const -> mir::optional_value<std::string> const&;

    /// The output id of the window.
    ///
    /// \returns a const reference to the output id
    /// \sa miral::Output - the class that holds this output id
    auto output_id() const -> mir::optional_value<int> const&;

    /// The type of the window.
    ///
    /// \returns a const reference to the type
    auto type() const -> mir::optional_value<MirWindowType> const&;

    /// The state of the window.
    ///
    /// \returns a const reference to the state
    auto state() const -> mir::optional_value<MirWindowState> const&;

    //// The preferred orientation of the window.
    ///
    /// This is often used when the buffer of the window is provided by the client to match
    /// the current orientation of the output.
    ///
    /// \returns a a const reference to the orientation.
    auto preferred_orientation() const -> mir::optional_value<MirOrientationMode> const&;

    /// Defines the rectangle of a parent window against which the #aux_rect_placement_gravity
    /// is decided.
    ///
    /// This is useful for when the window is of type #mir_window_type_menu, #mir_window_type_satellite
    /// or #mir_window_type_tip and wants to orient itself relative to a particular rectangle of the parent.
    ///
    /// \returns a const reference to the auxiliary rectangle
    auto aux_rect() const -> mir::optional_value<Rectangle> const&;

    /// The placement hint describes how child windows should be adjusted when
    /// their placement would cause them to extend beyond their current output.
    ///
    /// \returns a const reference to the placement hints
    auto placement_hints() const -> mir::optional_value<MirPlacementHints> const&;

    /// The placement gravity describes what edge of a popup window should attach
    /// to a corresponding edge of its parent.
    ///
    /// This only applies to windows with type #mir_window_type_menu, #mir_window_type_satellite
    /// or #mir_window_type_tip.
    ///
    /// #aux_rect_placement_gravity provides a way to set a corresponding point on a parent,
    /// such that the edge defined by the #window_placement_gravity of the popup touches
    /// the edge defined by the #aux_rect_placement_gravity of the parent window.
    ///
    /// \returns a const reference to the placement gravity
    auto window_placement_gravity() const -> mir::optional_value<MirPlacementGravity> const&;

    /// The auxiliary placement gravity of the window
    ///
    /// \returns a const reference to the placement gravity
    auto aux_rect_placement_gravity() const -> mir::optional_value<MirPlacementGravity> const&;

    /// The auxiliary placement describes the edge of the parent that the corresponding edge
    /// of the popup should attach itself to.
    ///
    /// This only applied to windows with type #mir_window_type_menu, #mir_window_type_satellite
    /// or #mir_window_type_tip.
    ///
    /// #window_placement_gravity provides a way to set a corresponding point on the popup itself,
    /// such that the edge defined by the #window_placement_gravity of the popup touches
    /// the edge defined by the #aux_rect_placement_gravity of the parent window.
    ///
    /// \returns a const reference to the placement offset
    auto aux_rect_placement_offset() const -> mir::optional_value<Displacement> const&;

    /// The minimum width of the window.
    ///
    /// \returns a const reference to the minimum width
    auto min_width() const -> mir::optional_value<Width> const&;

    /// The minimum height of the window.
    ///
    /// \returns a const reference to the minimum height
    auto min_height() const -> mir::optional_value<Height> const&;

    /// The maximum width of the window.
    ///
    /// \returns a const reference to the maximum width
    auto max_width() const -> mir::optional_value<Width> const&;

    /// The maximum height of the window.
    ///
    /// \returns a const refence to the maximum height
    auto max_height() const -> mir::optional_value<Height> const&;

    /// The size increments of the window in the X direction.
    ///
    /// This is used in cases such as a terminal that can only be resized
    /// character-by-character.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// \returns a const reference to the width increment.
    auto width_inc() const -> mir::optional_value<DeltaX> const&;

    /// The size increments of the window in the Y direction.
    ///
    /// This is used in cases such as a terminal that can only be resized
    /// character-by-character.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// \returns a const reference to the height increment
    auto height_inc() const -> mir::optional_value<DeltaY> const&;

    /// The minimum aspect ratio.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// \returns a const reference to the min aspect ratio
    auto min_aspect() const -> mir::optional_value<AspectRatio> const&;

    /// The maximum aspect ratio.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// \returns a const reference to the max aspect ratio
    auto max_aspect() const -> mir::optional_value<AspectRatio> const&;

    /// The parent surface of the window.
    ///
    /// \returns the pending parent surface
    auto parent() const -> mir::optional_value<std::weak_ptr<mir::scene::Surface>> const&;

    /// The input shape of the window.
    ///
    /// This is an area in world coordinates that describes the input region
    /// for the window.
    ///
    /// \returns a const reference to the list of rectangles that describes the input region
    auto input_shape() const -> mir::optional_value<std::vector<Rectangle>> const&;

    /// The input mode of the window.
    ///
    /// \returns a const reference to the input mode
    auto input_mode() const -> mir::optional_value<InputReceptionMode> const&;

    /// The shell chrome of the window.
    ///
    /// This is currently unused.
    ///
    /// \returns a const reference to the shell chrome
    auto shell_chrome() const -> mir::optional_value<MirShellChrome> const&;

    /// The pointer confinement of the window.
    ///
    /// \returns a const reference to the pointer confinement
    auto confine_pointer() const -> mir::optional_value<MirPointerConfinementState> const&;

    /// Custom userdata set on the window.
    ///
    /// This payload is set by the compositor author. The author may use this to
    /// set any information that they would like associated with the window.
    ///
    /// \returns a const reference to the userdata
    auto userdata() const -> mir::optional_value<std::shared_ptr<void>> const&;

    /// The new position of the window frame.
    ///
    /// \returns a reference to  the top left point
    auto top_left() -> mir::optional_value<Point>&;

    /// The size of the window frame, including any decorations.
    ///
    /// This value will be adjusted based on #min_width(), #WindowInfo::max_width(),
    /// #WindowInfo::min_height(), #WindowInfo::max_height(), #WindowInfo::min_aspect(),
    /// #WindowInfo::max_aspect(), #WindowInfo::width_inc() and WindowInfo::height_inc().
    /// Set these properties to their default values if they should be ignored.
    /// Note that the position of the window may also be adjusted if the new size violates
    /// the size constraints.
    ///
    /// This value is **not** guaranteed to be honored by the client.
    ///
    /// \returns a reference to the size
    auto size() -> mir::optional_value<Size>&;

    /// The name of the window.
    ///
    /// \returns a reference to the name of the window
    auto name() -> mir::optional_value<std::string>&;

    /// The output id of the window.
    ///
    /// \returns a reference to the output id
    /// \sa miral::Output - the class that holds this output id
    auto output_id() -> mir::optional_value<int>&;

    /// The type of the window.
    ///
    /// \returns a reference to the type of the window
    auto type() -> mir::optional_value<MirWindowType>&;

    /// The state of the window.
    ///
    /// \returns a reference to the state of the window
    auto state() -> mir::optional_value<MirWindowState>&;

    /// The preferred orientation of the window.
    ///
    /// This is often used when the buffer of the window is provided by the client to match
    /// the current orientation of the output.
    ///
    /// \returns a reference to the orientation of the window.
    auto preferred_orientation() -> mir::optional_value<MirOrientationMode>&;

    /// Defines the rectangle of a parent window against which the #aux_rect_placement_gravity
    /// is decided.
    ///
    /// This is useful for when the window is of type #mir_window_type_menu, #mir_window_type_satellite
    /// or #mir_window_type_tip and wants to orient itself relative to a particular rectangle of the parent.
    ///
    /// \returns a reference to the auxiliary rectangle
    auto aux_rect() -> mir::optional_value<Rectangle>&;

    /// The placement hint describes how windows with type #mir_window_type_menu,
    /// #mir_window_type_satellite or #mir_window_type_tip should be adjusted when
    /// their placement would cause them to extend beyond their current output.
    ///
    /// \returns a reference to the placement hints
    auto placement_hints() -> mir::optional_value<MirPlacementHints>&;

    /// The placement gravity describes what edge of a popup window should attach
    /// to a corresponding edge of its parent.
    ///
    /// This only applies to windows with type #mir_window_type_menu, #mir_window_type_satellite
    /// or #mir_window_type_tip.
    ///
    /// #aux_rect_placement_gravity provides a way to set a corresponding point on a parent,
    /// such that the edge defined by the #window_placement_gravity of the popup touches
    /// the edge defined by the #aux_rect_placement_gravity of the parent window.
    ///
    /// \returns  a reference to the window placement gravity
    auto window_placement_gravity() -> mir::optional_value<MirPlacementGravity>&;

    /// The auxiliary placement describes the edge of the parent that the corresponding edge
    /// of the popup should attach itself to.
    ///
    /// This only applied to windows with type #mir_window_type_menu, #mir_window_type_satellite
    /// or #mir_window_type_tip.
    ///
    /// #window_placement_gravity provides a way to set a corresponding point on the popup itself,
    /// such that the edge defined by the #window_placement_gravity of the popup touches
    /// the edge defined by the #aux_rect_placement_gravity of the parent window.
    ///
    /// \returns a reference to the auxiliary placement gravity
    auto aux_rect_placement_gravity() -> mir::optional_value<MirPlacementGravity>&;

    // The auxiliary placement describes the edge of the parent that the corresponding edge
    /// of the popup should attach itself to.
    ///
    /// This only applied to windows with type #mir_window_type_menu, #mir_window_type_satellite
    /// or #mir_window_type_tip.
    ///
    /// #window_placement_gravity provides a way to set a corresponding point on the popup itself,
    /// such that the edge defined by the #window_placement_gravity of the popup touches
    /// the edge defined by the #aux_rect_placement_gravity of the parent window.
    ///
    /// \returns a reference to the placement offset
    auto aux_rect_placement_offset() -> mir::optional_value<Displacement>&;

    /// The minimum width of the window.
    ///
    /// \returns a reference to the minimum width
    auto min_width() -> mir::optional_value<Width>&;

    /// The minimum height of the window.
    ///
    /// \returns a reference to the minimum height
    auto min_height() -> mir::optional_value<Height>&;

    /// The maximum width of the window.
    ///
    /// \returns a reference to the maximum width
    auto max_width() -> mir::optional_value<Width>&;

    /// The maximum height of the window
    ///
    /// \returns a refence to the maximum height
    auto max_height() -> mir::optional_value<Height>&;

    /// The size increments of the window in the X direction.
    ///
    /// This is used in cases such as a terminal that can only be resized
    /// character-by-character.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// \returns a reference to the width increment
    auto width_inc() -> mir::optional_value<DeltaX>&;

    /// The size increments of the window in the Y direction.
    ///
    /// This is used in cases such as a terminal that can only be resized
    /// character-by-character.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// \returns a reference to the height increment
    auto height_inc() -> mir::optional_value<DeltaY>&;

    /// The minimum aspect ratio.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// \returns a reference to the min aspect ratio
    auto min_aspect() -> mir::optional_value<AspectRatio>&;

    /// The maximum aspect ratio.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// \returns a reference to the max aspect ratio
    auto max_aspect() -> mir::optional_value<AspectRatio>&;

    /// The parent of this window.
    ///
    /// \returns a reference to the parent of this window
    auto parent() -> mir::optional_value<std::weak_ptr<mir::scene::Surface>>&;

    /// The input shape of the window.
    ///
    /// This is an area in world coordinates that describes the input region
    /// for the window.
    ///
    /// \returns a reference to the list of rectangles that describes the input region
    auto input_shape() -> mir::optional_value<std::vector<Rectangle>>&;

    /// The input mode of the window.
    ///
    /// \returns a reference to the input mode
    auto input_mode() -> mir::optional_value<InputReceptionMode>&;

    /// The shell chrome of the window.
    ///
    /// This is currently unused.
    ///
    /// \returns a reference to the shell chrome
    auto shell_chrome() -> mir::optional_value<MirShellChrome>&;

    /// The pointer confinement of the window.
    ///
    /// \returns a reference to the pointer confinement
    auto confine_pointer() -> mir::optional_value<MirPointerConfinementState>&;

    /// Custom userdata set on the window.
    ///
    /// This payload is set by the compositor author. The author may use this to
    /// set any information that they would like associated with the window.
    ///
    /// \returns a reference to the userdata
    auto userdata() -> mir::optional_value<std::shared_ptr<void>>&;

    /// The depth layer of a child window is updated with the depth layer of its parent, but can be overridden.
    ///
    /// \returns a const reference to the depth layer
    auto depth_layer() const -> mir::optional_value<MirDepthLayer> const&;

    /// The depth layer of a child window is updated with the depth layer of its parent, but can be overridden.
    ///
    /// \returns a reference to the depth layer
    auto depth_layer() -> mir::optional_value<MirDepthLayer>&;

    /// The set of window edges that are attached to edges of the output.
    ///
    /// If attached to perpendicular edges, it is attached to the corner where the two edges intersect
    /// If attached to oposite edges (eg left and right), it is stretched across the output in that direction
    /// If all edges are specified, it takes up the entire output
    ///
    /// \returns a const reference to the edges
    auto attached_edges() const -> mir::optional_value<MirPlacementGravity> const&;

    /// The set of window edges that are attached to edges of the output.
    ///
    /// If attached to perpendicular edges, it is attached to the corner where the two edges intersect
    /// If attached to oposite edges (eg left and right), it is stretched across the output in that direction
    /// If all edges are specified, it takes up the entire output
    ///
    /// \returns a reference to the edges
    auto attached_edges() -> mir::optional_value<MirPlacementGravity>&;

    /// The area over which the window should not be occluded.
    ///
    /// This is only meaningful for windows attached to an edge.
    /// If the outer optional is unset (the default), the window's exclusive rect is not changed by this spec
    /// If the outer optional is set but the inner is not, the window's exclusive rect is cleared
    ///
    /// \returns a const reference to the exclusive rectangle
    auto exclusive_rect() const -> mir::optional_value<mir::optional_value<mir::geometry::Rectangle>> const&;

    /// The area over which the window should not be occluded.
    ///
    /// This is only meaningful for windows attached to an edge.
    /// If the outer optional is unset (the default), the window's exclusive rect is not changed by this spec
    /// If the outer optional is set but the inner is not, the window's exclusive rect is cleared
    ///
    /// \returns a reference to the exclusive rectangle
    auto exclusive_rect() -> mir::optional_value<mir::optional_value<mir::geometry::Rectangle>>&;

    /// Decides whether this window should ignore the exclusion zones set by other windows.
    ///
    /// This is only meaningful for windows attached to an edge.
    ///
    /// \returns a const reference to the flag
    auto ignore_exclusion_zones() const -> mir::optional_value<bool> const&;

    /// Decides whether this window should ignore the exclusion zones set by other windows.
    ///
    /// This is only meaningful for windows attached to an edge.
    ///
    /// \returns a reference to the flag
    auto ignore_exclusion_zones() -> mir::optional_value<bool>&;

    /// The D-bus service name and basename of the app's .desktop file.
    ///
    /// See https://specifications.freedesktop.org/desktop-entry-spec/
    ///
    /// \returns a const reference to the application id
    auto application_id() const -> mir::optional_value<std::string> const&;

    /// The D-bus service name and basename of the app's .desktop file.
    ///
    /// See https://specifications.freedesktop.org/desktop-entry-spec/
    ///
    /// \returns a reference to the application id
    auto application_id() -> mir::optional_value<std::string>&;

    /// If this window should have server-side decorations provided by Mir
    /// Currently, Mir only respects this value during surface construction
    ///
    /// \returns a const reference to the flag
    auto server_side_decorated() const -> mir::optional_value<bool> const&;

    /// If this window should have server-side decorations provided by Mir
    /// Currently, Mir only respects this value during surface construction
    ///
    /// \returns a reference to the flag
    auto server_side_decorated() -> mir::optional_value<bool>&;

    /// Describes how the window should gain and lose focus.
    ///
    /// \returns a const reference to the focus mode
    auto focus_mode() const -> mir::optional_value<MirFocusMode> const&;

    /// Describes how the window should gain and lose focus.
    ///
    /// \returns a reference to the focus mode
    auto focus_mode() -> mir::optional_value<MirFocusMode>&;

    /// If this surface should be shown while the compositor is locked.
    ///
    /// \returns a const reference to the flag
    auto visible_on_lock_screen() const -> mir::optional_value<bool> const&;

    /// If this surface should be shown while the compositor is locked.
    ///
    /// \returns a reference to the flag
    auto visible_on_lock_screen() -> mir::optional_value<bool>&;

    /// Describes which edges are touching part of the tiling grid.
    ///
    /// \returns a const reference to the tiled edges
    /// \remark Since MirAL 5.3
    auto tiled_edges() const -> mir::optional_value<mir::Flags<MirTiledEdge>> const&;

    /// Describes which edges are touching part of the tiling grid.
    ///
    /// \returns a reference to the tiled edges
    /// \remark Since MirAL 5.3
    auto tiled_edges() -> mir::optional_value<mir::Flags<MirTiledEdge>>&;

    /// Create a [mir::shell::SurfaceSpecification] from this window spec.
    ///
    /// \returns a surface specification
    /// \remark Since MirAL 5.3
    auto to_surface_specification() const -> mir::shell::SurfaceSpecification;

private:
    friend auto make_surface_spec(WindowSpecification const& miral_spec) -> mir::shell::SurfaceSpecification;
    struct Self;
    std::unique_ptr<Self> self;
};
}

#endif //MIRAL_WINDOW_SPECIFICATION_H
