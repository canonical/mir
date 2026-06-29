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

#ifndef MIRAL_TOUCH_EMULATOR_H
#define MIRAL_TOUCH_EMULATOR_H

#include <memory>

namespace mir { class Server; }

namespace miral
{

/// Translates left-button pointer drag events into single-contact touch events.
///
/// This is a development and testing aid that allows touch-oriented features to
/// be exercised on pointer-only desktops (e.g. when running nested in a desktop
/// session without a physical touchscreen).
///
/// Mapping:
///   Left button down  → mir_touch_action_down
///   Motion while down → mir_touch_action_change
///   Left button up    → mir_touch_action_up
///
/// Holding Ctrl while dragging suppresses translation so normal pointer
/// interaction is still reachable via Ctrl+drag.
///
/// \remark Since MirAL 5.5
class TouchEmulator
{
public:
    TouchEmulator();

    void operator()(mir::Server& server);

private:
    class Self;
    std::shared_ptr<Self> self;
};

} // namespace miral

#endif // MIRAL_TOUCH_EMULATOR_H
