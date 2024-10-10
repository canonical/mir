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

#ifndef MIRAL_WINDOW_INFO_H
#define MIRAL_WINDOW_INFO_H

#include "miral/window.h"
#include "miral/window_specification.h"

#include <mir/geometry/rectangles.h>
#include <mir/optional_value.h>

#include <algorithm>

namespace miral
{
struct WindowInfo
{
    using AspectRatio = WindowSpecification::AspectRatio;

    WindowInfo();
    WindowInfo(Window const& window, WindowSpecification const& params);
    ~WindowInfo();
    explicit WindowInfo(WindowInfo const& that);
    WindowInfo& operator=(WindowInfo const& that);

    bool can_be_active() const;

    bool can_morph_to(MirWindowType new_type) const;

    bool must_have_parent() const;

    bool must_not_have_parent() const;

    bool is_visible() const;

    /// \deprecated Obsolete: Window::size() includes decorations
    /// @{
    [[deprecated("Obsolete: Window::size() includes decorations")]]
    static bool needs_titlebar(MirWindowType type);
    /// @}

    void constrain_resize(mir::geometry::Point& requested_pos, mir::geometry::Size& requested_size) const;

    auto window() const -> Window&;

    auto name() const -> std::string;

    auto type() const -> MirWindowType;

    auto state() const -> MirWindowState;

    auto restore_rect() const -> mir::geometry::Rectangle;

    auto parent() const -> Window;

    auto children() const -> std::vector <Window> const&;

    /// These constrain the sizes a window may be resized to (both interactively and pragmatically). Clients can request
    /// a min/max size, but it can be overridden by the window management policy. By default, minimum values are 0 and
    /// maximum values are `std::numeric_limits<int>::max()`.
    /// @{
    auto min_width() const -> mir::geometry::Width;
    auto min_height() const -> mir::geometry::Height;
    auto max_width() const -> mir::geometry::Width;
    auto max_height() const -> mir::geometry::Height;
    /// @}

    /// These control the size increments of the window. This is used in cases like a terminal that can only be resized
    /// character-by-character. Current Wayland protocols do not support this property, so it generally wont be
    /// requested by clients. By default, both are 1.
    /// @{
    auto width_inc() const -> mir::geometry::DeltaX;
    auto height_inc() const -> mir::geometry::DeltaY;
    /// @}

    /// These constrain the possible aspect ratio of the window. Current Wayland protocols to not support this property,
    /// so it generally wont be requested by clients. By default, min_aspect is
    /// `{0U, std::numeric_limits<unsigned>::max()}` and max_aspect is `{std::numeric_limits<unsigned>::max(), 0U}`.
    /// @{
    auto min_aspect() const -> AspectRatio;
    auto max_aspect() const -> AspectRatio;
    /// @}

    bool has_output_id() const;
    auto output_id() const -> int;

    auto preferred_orientation() const -> MirOrientationMode;

    auto confine_pointer() const -> MirPointerConfinementState;

    auto shell_chrome() const -> MirShellChrome;

    /// This can be used by client code to store window manager specific information
    auto userdata() const -> std::shared_ptr<void>;
    void userdata(std::shared_ptr<void> userdata);

    void swap(WindowInfo& rhs) { std::swap(self, rhs.self); }

    auto depth_layer() const -> MirDepthLayer;

    /// Get the edges of the output that the window is attached to
    /// (only meaningful for windows in state mir_window_state_attached)
    auto attached_edges() const -> MirPlacementGravity;

    /// Mir will try to avoid occluding the area covered by this rectangle (relative to the window)
    /// (only meaningful when the window is attached to an edge)
    auto exclusive_rect() const -> mir::optional_value<mir::geometry::Rectangle>;

    /// Mir will ignore the exclusive_rects of other windows when this is set to true.
    /// (only meaningful when the window is attached to an edge)
    auto ignore_exclusion_zones() const -> bool;

    /// Mir will not render anything outside this rectangle
    auto clip_area() const -> mir::optional_value<mir::geometry::Rectangle>;
    void clip_area(mir::optional_value<mir::geometry::Rectangle> const& area);

    /// The D-bus service name and basename of the app's .desktop file
    /// See https://specifications.freedesktop.org/desktop-entry-spec/
    /// \remark Since MirAL 2.8
    ///@{
    auto application_id() const -> std::string;
    ///@}

    /// How the window should gain and lose focus
    /// \remark Since MirAL 3.3
    auto focus_mode() const -> MirFocusMode;

    /// If this surface should be shown while the compositor is locked
    /// \remark Since MirAL 3.9
    auto visible_on_lock_screen() const -> bool;

private:
    friend class BasicWindowManager;
    void name(std::string const& name);
    void type(MirWindowType type);
    void state(MirWindowState state);
    void restore_rect(mir::geometry::Rectangle const& restore_rect);
    void parent(Window const& parent);
    void add_child(Window const& child);
    void remove_child(Window const& child);
    void min_width(mir::geometry::Width min_width);
    void min_height(mir::geometry::Height min_height);
    void max_width(mir::geometry::Width max_width);
    void max_height(mir::geometry::Height max_height);
    void width_inc(mir::geometry::DeltaX width_inc);
    void height_inc(mir::geometry::DeltaY height_inc);
    void min_aspect(AspectRatio min_aspect);
    void max_aspect(AspectRatio max_aspect);
    void output_id(mir::optional_value<int> output_id);
    void preferred_orientation(MirOrientationMode preferred_orientation);
    void confine_pointer(MirPointerConfinementState confinement);
    void shell_chrome(MirShellChrome chrome);
    void depth_layer(MirDepthLayer depth_layer);
    void attached_edges(MirPlacementGravity edges);
    void exclusive_rect(mir::optional_value<mir::geometry::Rectangle> const& rect);
    void application_id(std::string const& application_id);
    void focus_mode(MirFocusMode focus_mode);
    void visible_on_lock_screen(bool visible);

    struct Self;
    std::unique_ptr<Self> self;
};
}

namespace std
{
template<> inline void swap(miral::WindowInfo& lhs, miral::WindowInfo& rhs) { lhs.swap(rhs); }
}

#endif //MIRAL_WINDOW_INFO_H
