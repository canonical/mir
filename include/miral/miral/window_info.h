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

#include <miral/window.h>
#include <miral/window_specification.h>

#include <mir/geometry/rectangles.h>
#include <mir/optional_value.h>
#include <mir/flags.h>

namespace miral
{
/// Provides additional information about a #miral::Window.
///
/// An instance of this class may be obtained from miral::WindowManagerTools::info_for.
///
/// \sa miral::Window - the class for which this class provides information
/// \sa miral::WindowManagerTools::info_for - the method to get an instance of this class
struct WindowInfo
{
    using AspectRatio = WindowSpecification::AspectRatio;

    /// Constructs a new window info instance not backed by a #miral::Window.
    ///
    /// This will result in all methods on this class returning a stub result.
    WindowInfo();

    /// Constructs a new window info instanced backed by \p window with the
    /// specified \p params.
    ///
    /// \param window the backing window
    /// \param params the initial specification for the window
    WindowInfo(Window const& window, WindowSpecification const& params);
    ~WindowInfo();
    explicit WindowInfo(WindowInfo const& that);
    WindowInfo& operator=(WindowInfo const& that);

    /// Check if the window can have focus.
    ///
    /// \returns `true` if it can be active, otherwise `false`
    bool can_be_active() const;

    /// Check if the window can change its type to \p new_type.
    ///
    /// \param new_type the desired type to change to
    /// \returns `true` if the window can change to the desired type, otherwise `false`
    bool can_morph_to(MirWindowType new_type) const;

    /// Check if the window must have a parent.
    ///
    /// \returns true if the window must have a parent, otherwise false
    bool must_have_parent() const;

    /// Check if the window must **not** have a parent.
    ///
    /// \returns true if the window must NOT have a parent, otherwise false.
    bool must_not_have_parent() const;

    /// Check if the window is currently visible.
    ///
    /// \returns `true` if the window is visible, otherwise `false`.
    bool is_visible() const;

    /// \deprecated Obsolete: Window::size() includes decorations
    /// @{
    [[deprecated("Obsolete: Window::size() includes decorations")]]
    static bool needs_titlebar(MirWindowType type);
    /// @}

    void constrain_resize(mir::geometry::Point& requested_pos, mir::geometry::Size& requested_size) const;

    /// The #miral::Window that backs this instance
    ///
    /// \returns the backing window
    auto window() const -> Window&;

    /// The name of the window.
    ///
    /// In Wayland, this is most commonly set by the client via `xdg_toplevel::set_title`,
    /// but the exact mechanism may vary depending on the client.
    ///
    /// \returns the name of the window
    auto name() const -> std::string;

    /// The type of the window.
    ///
    /// \returns type of the window
    /// \sa MirWindowType - describes the type of window
    auto type() const -> MirWindowType;

    /// Returns the state of the window.
    ///
    /// \returns state of the window
    /// \sa MirWindowState - describes the state that a window can be in
    auto state() const -> MirWindowState;

    /// The size that the window should return to after entering to the
    /// #mir_window_state_restored state.
    ///
    /// Before becoming hidden, fullscreen, maximized, or minimized, the window
    /// stores its previous rectangle so that its original size can easily be
    /// restored afterward. This size is returned by this method.
    ///
    /// \returns the restore rectangle
    auto restore_rect() const -> mir::geometry::Rectangle;

    /// The parent of this window.
    ///
    /// The result may be the default constructed window (i.e. the
    /// "null" window) in the event that this window lacks a parent.
    ///
    /// \returns the parent window
    auto parent() const -> Window;

    /// The children of this window, if any.
    ///
    /// \returns the children of this window
    auto children() const -> std::vector <Window> const&;

    /// The minimum width of the window.
    ///
    /// This can be set either by the client or by the compositor author.
    /// If set by the client, it is most likely set by `xdg_toplevel::set_min_size`.
    ///
    /// The compositor author may choose to ignore this value, but please be aware
    /// that clients may misbehave in such scenarios. For example, a client may continue
    /// sending buffers that adhere to the minimum width
    ///
    /// Defaults to 0.
    ///
    /// \returns the minimum width
    auto min_width() const -> mir::geometry::Width;

    /// The minimum height of the window.
    ///
    /// This can be set either by the client or by the compositor author.
    /// If set by the client, it is most likely set by `xdg_toplevel::set_min_size`.
    ///
    /// The compositor author may choose to ignore this value, but please be aware
    /// that clients may misbehave in such scenarios. For example, a client may continue
    /// sending buffers that adhere to the minimum height.
    ///
    /// Defaults to 0.
    ///
    /// \returns the minimum height
    auto min_height() const -> mir::geometry::Height;

    /// The maximum width of the window.
    ///
    /// This can be set either by the client or by the compositor author.
    /// If set by the client, it is most likely set by `xdg_toplevel::set_max_size`.
    ///
    /// The compositor author may choose to ignore this value, but please be aware
    /// that clients may misbehave in such scenarios. For example, a client may continue
    /// sending buffers that adhere to the minimum width
    ///
    /// Defaults to `std::numeric_limits<int>::max()`.
    ///
    /// \returns the maximum width
    auto max_width() const -> mir::geometry::Width;

    /// The maximum height of the window.
    ///
    /// This can be set either by the client or by the compositor author.
    /// If set by the client, it is most likely set by `xdg_toplevel::set_max_size`.
    ///
    /// The compositor author may choose to ignore this value, but please be aware
    /// that clients may misbehave in such scenarios. For example, a client may continue
    /// sending buffers that adhere to the minimum width
    ///
    /// Defaults to `std::numeric_limits<int>::max()`.
    ///
    /// \returns the maximum height
    auto max_height() const -> mir::geometry::Height;

    /// The size increments of the window in the X direction.
    ///
    /// This is used in cases such as a terminal that can only be resized
    /// character-by-character.
    ///
    /// Wayland protocols do not support this property, so it is generally not used by clients.
    ///
    /// Defaults to 1.
    ///
    /// \returns the width increment
    auto width_inc() const -> mir::geometry::DeltaX;

    /// The size increments of the window in the Y direction.
    ///
    /// This is used in cases such as a terminal that can only be resized
    /// character-by-character.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// Defaults to 1.
    ///
    /// \returns the width increment
    auto height_inc() const -> mir::geometry::DeltaY;

    /// The minimum aspect ratio.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// Defaults to `{0U, std::numeric_limits<unsigned>::max()}`.
    /// \returns the minimum aspect ratio
    auto min_aspect() const -> AspectRatio;

    /// The maximum aspect ratio.
    ///
    /// Wayland protocols do not support this property, so it generally will not be requested by clients.
    ///
    /// Defaults to `{std::numeric_limits<unsigned>::max(), 0U}`.
    /// \returns the maximum aspect ratio
    auto max_aspect() const -> AspectRatio;

    /// Whether the window is associated with a particular output.
    ///
    /// If this is `false`, then #miral::WindowInfo::output_id will throw
    /// a fatal error when accessed.
    ///
    /// \returns `true` if #output_id is valid, otherwise `false`
    bool has_output_id() const;

    /// The output id that this window is associated with.
    ///
    /// If #miral::WindoInfo::has_output_id is false, then this method
    /// will throw a fatal error when accessed.
    ///
    /// Callers may match this value with an id from #miral::Output::id()
    /// in order to find out more details about the output.
    ///
    /// \returns the output id, or a fatal error if none is set
    auto output_id() const -> int;

    /// The preferred orientation of the window.
    ///
    /// Defaults to #mir_orientation_mode_portrait.
    ///
    /// \returns the orienation of the window
    /// \sa MirOrientationMode - the orientations options
    auto preferred_orientation() const -> MirOrientationMode;

    /// The confinement of the pointer.
    ///
    /// Defaults to #mir_pointer_unconfined.
    ///
    /// \returns the confinement of the pointer
    /// \sa MirPointerConfinementState - the pointer confinement options
    auto confine_pointer() const -> MirPointerConfinementState;

    /// The shell chrome type.
    ///
    /// Defaults to #mir_shell_chrome_normal.
    ///
    /// \returns the shell chrome type
    /// \sa MirShellChrome - the shell chrome options
    auto shell_chrome() const -> MirShellChrome;

    /// Custom user data for the window.
    ///
    /// This can be set by the compositor author via #miral::WindowInfo::userdata(std::shared_ptr<void>).
    /// This is an arbitrary payload that can contains compositor-specific information.
    ///
    /// \returns the userdata, or nullptr if none was set
    auto userdata() const -> std::shared_ptr<void>;

    /// Set the custom user data for the window.
    ///
    /// This can be accessed by #miral::WindowInfo::userdata().
    /// This is an arbitrary payload that can contain compositor-specific information.
    ///
    /// \param userdata the userdata
    void userdata(std::shared_ptr<void> userdata);

    /// Swap the backing information of \rhs with the contents
    /// held in this object.
    void swap(WindowInfo& rhs) { std::swap(self, rhs.self); }

    /// The depth layer of the window.
    ///
    /// This can be requested by the client or set by the compositor author.
    /// If set by a client, this often comes from `zwlr_layer_surface_v1::set_layer`.
    ///
    /// \returns the depth layer
    /// \sa MirDepthLayer - the depth layer options
    auto depth_layer() const -> MirDepthLayer;

    /// Get the edges of the output that the window is attached to
    ///
    /// This value is only meaningful for windows when #miral::WindowInfo::state
    /// is set to #mir_window_state_attached.
    ///
    /// \returns the placement gravity
    auto attached_edges() const -> MirPlacementGravity;

    /// Describes a rectangular area that Mir will avoid occluding.
    ///
    /// This area is relative to the window.
    ///
    /// This value is only meaningful for windows when #miral::WindowInfo::state
    /// is set to #mir_window_state_attached.
    ///
    /// \returns the exclusive rect optional
    auto exclusive_rect() const -> mir::optional_value<mir::geometry::Rectangle>;

    /// When `true`, this window will ignore the #miral::WindowInfo::exclusive_rect of
    /// other windows.
    ///
    /// This value is only meaningful for windows when #miral::WindowInfo::state
    /// is set to #mir_window_state_attached.
    ///
    /// \returns `true` if exclusion zones are ignored, otherwise `false`
    auto ignore_exclusion_zones() const -> bool;

    /// The clip area for the window.
    ///
    /// If set, Mir will not render any part of the window that falls outside
    /// of this rectangle. Compositor authors can set this via
    /// #miral::WindowInfo::clip_area(mir::optional_value<mir::geometry::Rectangle>).
    ///
    /// This rectangle is in world coordinates.
    ///
    /// \returns the clip area set for the window
    auto clip_area() const -> mir::optional_value<mir::geometry::Rectangle>;

    /// Set the clip area of the window.
    ///
    /// If set, Mir will not render any part of the window that falls outside
    /// of this rectangle.
    ///
    /// This rectangle is in world coordinates.
    ///
    /// \param area the rectangle for the clip area
    void clip_area(mir::optional_value<mir::geometry::Rectangle> const& area);

    /// The D-bus service name and basename of the app's .desktop
    ///
    /// See https://specifications.freedesktop.org/desktop-entry-spec/.
    ///
    /// \returns the application id
    auto application_id() const -> std::string;

    /// Describes how the window should gain and lose focus.
    ///
    /// \returns the focus mode
    /// \sa MirFocusMode - the focus mode options
    auto focus_mode() const -> MirFocusMode;

    /// If this surface should be shown while the compositor is locked
    ///
    /// \returns `true` if it will be shown on the lock screen, otherwise `false`
    auto visible_on_lock_screen() const -> bool;

    /// Describes which edges the window is tiled against.
    ///
    /// Used when the surface is in a tiled layout to describe the
    /// edges that are touching another part of the tiling grid.
    ///
    /// \returns a flag containing the tiled edges that are set
    /// \remark Since MirAL 5.3
    /// \sa MirTiledEdges - the tiled edge options
    auto tiled_edges() const -> mir::Flags<MirTiledEdge>;

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
    void tiled_edges(mir::Flags<MirTiledEdge> flags);

    struct Self;
    std::unique_ptr<Self> self;
};
}

namespace std
{
template<> inline void swap(miral::WindowInfo& lhs, miral::WindowInfo& rhs) { lhs.swap(rhs); }
}

#endif //MIRAL_WINDOW_INFO_H
