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

#ifndef MIRAL_HIDE_CURSOR_H
#define MIRAL_HIDE_CURSOR_H

#include <chrono>
#include <memory>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }

/// Hides the cursor while a pointing device is attached but not in use.
///
/// Each hide trigger can be enabled independently; all are disabled by default.
/// A hidden cursor is shown again by a pointer button press, a scroll, or pointer
/// motion beyond the reveal threshold. The cursor is not hidden while a pointer
/// button is held.
///
/// \remark Since MirAL 6.2
class HideCursor
{
public:
    /// Construct with all hide triggers disabled.
    HideCursor();

    /// Construct registering with a configuration store
    ///
    /// Available options:
    ///     - {cursor, hide_on_touch}: Hide the cursor on touch-down. Defaults to false.
    ///     - {cursor, hide_on_keyboard}: Hide the cursor on key press. Defaults to false.
    ///     - {cursor, hide_on_inactivity}: Hide the cursor after pointer inactivity. Defaults to false.
    ///     - {cursor, inactivity_timeout_ms}: Pointer inactivity timeout in milliseconds. Defaults to 5000.
    ///     - {cursor, reveal_threshold}: Pointer motion, in logical pixels, needed to show
    ///       a hidden cursor. Defaults to 10.
    ///
    /// \param config_store the config store
    explicit HideCursor(live_config::Store& config_store);

    ~HideCursor();

    /// Hide the cursor on touch-down.
    HideCursor& on_touch(bool enable);

    /// Hide the cursor on key press.
    HideCursor& on_keyboard(bool enable);

    /// Hide the cursor after the inactivity timeout without pointer events.
    HideCursor& on_inactivity(bool enable);

    /// Set the pointer inactivity timeout. Negative values are treated as zero.
    HideCursor& inactivity_timeout(std::chrono::milliseconds timeout);

    /// Set the pointer motion, in logical pixels, needed to show a hidden cursor.
    /// Negative values are treated as zero.
    HideCursor& reveal_threshold(float logical_pixels);

    void operator()(mir::Server& server) const;

private:
    struct Self;
    std::shared_ptr<Self> const self;
};
}

#endif // MIRAL_HIDE_CURSOR_H
