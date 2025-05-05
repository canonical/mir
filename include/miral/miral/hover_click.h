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
class HoverClick
{
public:
    HoverClick(bool enabled_by_default);

    void operator()(mir::Server& server);

    HoverClick& enable();
    HoverClick& disable();

    HoverClick& hover_duration(std::chrono::milliseconds hover_duration);
    HoverClick& cancel_displacement_threshold(float displacement);

    HoverClick& on_enabled(std::function<void()>&&);
    HoverClick& on_disabled(std::function<void()>&&);
    HoverClick& on_hover_start(std::function<void()>&&);
    HoverClick& on_hover_cancel(std::function<void()>&&);
    HoverClick& on_click_dispatched(std::function<void()>&&);

private:
    struct Self;
    std::shared_ptr<Self> const self;
};
}

#endif
