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

class WindowSpecification
{
public:
    enum class InputReceptionMode
    {
        normal,
        receives_all_input
    };

    struct AspectRatio { unsigned width; unsigned height; };

    WindowSpecification();
    WindowSpecification(WindowSpecification const& that);
    auto operator=(WindowSpecification const& that) -> WindowSpecification&;

    WindowSpecification(mir::shell::SurfaceSpecification const& spec);

    ~WindowSpecification();

    auto top_left() const -> mir::optional_value<Point> const&;
    auto size() const -> mir::optional_value<Size> const&;
    auto name() const -> mir::optional_value<std::string> const&;
    auto output_id() const -> mir::optional_value<int> const&;
    auto type() const -> mir::optional_value<MirWindowType> const&;
    auto state() const -> mir::optional_value<MirWindowState> const&;
    auto preferred_orientation() const -> mir::optional_value<MirOrientationMode> const&;
    auto aux_rect() const -> mir::optional_value<Rectangle> const&;
    auto placement_hints() const -> mir::optional_value<MirPlacementHints> const&;
    auto window_placement_gravity() const -> mir::optional_value<MirPlacementGravity> const&;
    auto aux_rect_placement_gravity() const -> mir::optional_value<MirPlacementGravity> const&;
    auto aux_rect_placement_offset() const -> mir::optional_value<Displacement> const&;
    auto min_width() const -> mir::optional_value<Width> const&;
    auto min_height() const -> mir::optional_value<Height> const&;
    auto max_width() const -> mir::optional_value<Width> const&;
    auto max_height() const -> mir::optional_value<Height> const&;
    auto width_inc() const -> mir::optional_value<DeltaX> const&;
    auto height_inc() const -> mir::optional_value<DeltaY> const&;
    auto min_aspect() const -> mir::optional_value<AspectRatio> const&;
    auto max_aspect() const -> mir::optional_value<AspectRatio> const&;

    auto parent() const -> mir::optional_value<std::weak_ptr<mir::scene::Surface>> const&;
    auto input_shape() const -> mir::optional_value<std::vector<Rectangle>> const&;
    auto input_mode() const -> mir::optional_value<InputReceptionMode> const&;
    auto shell_chrome() const -> mir::optional_value<MirShellChrome> const&;
    auto confine_pointer() const -> mir::optional_value<MirPointerConfinementState> const&;
    auto userdata() const -> mir::optional_value<std::shared_ptr<void>> const&;

    auto top_left() -> mir::optional_value<Point>&;
    /// The new size of the window frame (including any decorations). Will be adjusted based on min_width(),
    /// WindowInfo::max_width(), WindowInfo::min_height(), WindowInfo::max_height(), WindowInfo::min_aspect(),
    /// WindowInfo::max_aspect(), WindowInfo::width_inc() and WindowInfo::height_inc(). Set these to properties to their
    /// default values if they should be ignored.
    auto size() -> mir::optional_value<Size>&;
    auto name() -> mir::optional_value<std::string>&;
    auto output_id() -> mir::optional_value<int>&;
    auto type() -> mir::optional_value<MirWindowType>&;
    auto state() -> mir::optional_value<MirWindowState>&;
    auto preferred_orientation() -> mir::optional_value<MirOrientationMode>&;
    /// Relative to window's surface's CONTENT offset and size
    /// (not equal to the top_left and size exposed by this interface if server-side decorations are in use)
    auto aux_rect() -> mir::optional_value<Rectangle>&;
    auto placement_hints() -> mir::optional_value<MirPlacementHints>&;
    auto window_placement_gravity() -> mir::optional_value<MirPlacementGravity>&;
    auto aux_rect_placement_gravity() -> mir::optional_value<MirPlacementGravity>&;
    auto aux_rect_placement_offset() -> mir::optional_value<Displacement>&;

    /// Constrains how a window can be resized, see the corresponding properties on WindowInfo for details.
    /// @{
    auto min_width() -> mir::optional_value<Width>&;
    auto min_height() -> mir::optional_value<Height>&;
    auto max_width() -> mir::optional_value<Width>&;
    auto max_height() -> mir::optional_value<Height>&;
    auto width_inc() -> mir::optional_value<DeltaX>&;
    auto height_inc() -> mir::optional_value<DeltaY>&;
    auto min_aspect() -> mir::optional_value<AspectRatio>&;
    auto max_aspect() -> mir::optional_value<AspectRatio>&;
    /// @}

    auto parent() -> mir::optional_value<std::weak_ptr<mir::scene::Surface>>&;
    auto input_shape() -> mir::optional_value<std::vector<Rectangle>>&;
    auto input_mode() -> mir::optional_value<InputReceptionMode>&;
    auto shell_chrome() -> mir::optional_value<MirShellChrome>&;
    auto confine_pointer() -> mir::optional_value<MirPointerConfinementState>&;
    auto userdata() -> mir::optional_value<std::shared_ptr<void>>&;

    /// The depth layer of a child window is updated with the depth layer of its parent, but can be overridden
    ///@{
    auto depth_layer() const -> mir::optional_value<MirDepthLayer> const&;
    auto depth_layer() -> mir::optional_value<MirDepthLayer>&;
    ///@}

    /// The set of window eges that are attched to edges of the output
    /// If attached to perpendicular edges, it is attached to the corner where the two edges intersect
    /// If attached to oposite edges (eg left and right), it is stretched across the output in that direction
    /// If all edges are specified, it takes up the entire output
    ///@{
    auto attached_edges() const -> mir::optional_value<MirPlacementGravity> const&;
    auto attached_edges() -> mir::optional_value<MirPlacementGravity>&;
    ///@}

    /// The area over which the window should not be occluded
    /// Only meaningful for windows attached to an edge
    /// If the outer optional is unset (the default), the window's exclusive rect is not changed by this spec
    /// If the outer optional is set but the inner is not, the window's exclusive rect is cleared
    ///@{
    auto exclusive_rect() const -> mir::optional_value<mir::optional_value<mir::geometry::Rectangle>> const&;
    auto exclusive_rect() -> mir::optional_value<mir::optional_value<mir::geometry::Rectangle>>&;
    ///@}

    /// Decides whether or not this window should ignore the exclusion zones set by other windows.
    /// Only meaningful for windows attached to an edge
    ///@{
    auto ignore_exclusion_zones() const -> mir::optional_value<bool> const&;
    auto ignore_exclusion_zones() -> mir::optional_value<bool>&;
    ///@}

    /// The D-bus service name and basename of the app's .desktop file
    /// See https://specifications.freedesktop.org/desktop-entry-spec/
    ///@{
    auto application_id() const -> mir::optional_value<std::string> const&;
    auto application_id() -> mir::optional_value<std::string>&;
    ///@}

    /// If this window should have server-side decorations provided by Mir
    /// Currently, Mir only respects this value during surface construction
    ///@{
    auto server_side_decorated() const -> mir::optional_value<bool> const&;
    auto server_side_decorated() -> mir::optional_value<bool>&;
    ///@}

    /// How the window should gain and lose focus
    /// \remark Since MirAL 3.3
    ///@{
    auto focus_mode() const -> mir::optional_value<MirFocusMode> const&;
    auto focus_mode() -> mir::optional_value<MirFocusMode>&;
    ///@}

    /// If this surface should be shown while the compositor is locked
    /// \remark Since MirAL 3.9
    ///@{
    auto visible_on_lock_screen() const -> mir::optional_value<bool> const&;
    auto visible_on_lock_screen() -> mir::optional_value<bool>&;
    ///@}

    /// Describes which edges are touching part of the tiling grid
    /// \remark Since MirAL 5.3
    ///@{
    auto tiled_edges() const -> mir::optional_value<mir::Flags<MirTiledEdge>> const&;
    auto tiled_edges() -> mir::optional_value<mir::Flags<MirTiledEdge>>&;
    ///@}

    /// Create a [mir::shell::SurfaceSpecification] from this window spec.
    /// \remark Since MirAL 5.3
    auto to_surface_specification() const -> mir::shell::SurfaceSpecification;

private:
    friend auto make_surface_spec(WindowSpecification const& miral_spec) -> mir::shell::SurfaceSpecification;
    struct Self;
    std::unique_ptr<Self> self;
};
}

#endif //MIRAL_WINDOW_SPECIFICATION_H
