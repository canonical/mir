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

#ifndef MIRAL_HOVER_CLICK_H
#define MIRAL_HOVER_CLICK_H

namespace mir { class Server; }

#include <chrono>
#include <functional>

namespace miral
{
/// Enables configuring hover click at runtime.
///
/// Hover click is an accessibility feature that allows users to dispatch primary
/// clicks by holding the pointer still for some configurable period of time.
/// The pointer may move by a configurable distance to accommodate users with
/// disabilities. After a hover click, no further clicks are dispatched until
/// the cursor moves by a configurable distance from the position of the
/// previous click.
/// 
/// \remark Since MirAL 5.5
class HoverClick
{
public:
    explicit HoverClick(bool enabled_by_default);

    void operator()(mir::Server& server);

    /// Enables hover click.
    ///
    /// When already enabled, further calls have no effect.
    HoverClick& enable();

    /// Disables hover click.
    ///
    /// When already disabled, further calls have no effect.
    HoverClick& disable();

    /// Configures how long the pointer has to stay still to dispatch a left
    /// click.
    HoverClick& hover_duration(std::chrono::milliseconds hover_duration);

    // Configures the distance in pixels the pointer has to move from the
    // initial hover click position to cancel it.
    HoverClick& cancel_displacement_threshold(int displacement);
    
    // Configures the distance in pixels the pointer has to move from the the
    // last hover click or hover click cancel position to initiate a new hover
    // click
    HoverClick& reclick_displacement_threshold(int displacement);

    /// Called shortly after a hover click is scheduled. Should be used to
    /// indicate to the user that a hover click has begun.
    HoverClick& on_hover_start(std::function<void()>&&);

    // Called immediately when a hover click is cancelled. Should be used to
    // indicate to the user that the hover click was cancelled.
    HoverClick& on_hover_cancel(std::function<void()>&&);

    // Called immediately after a hover click is successfully dispatched.
    // Should be used to indicate to the user that the hover click was
    // successful.
    HoverClick& on_click_dispatched(std::function<void()>&&);

private:
    struct Self;
    std::shared_ptr<Self> const self;
};
}

#endif
