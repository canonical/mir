/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_CLIENT_WAYLAND_RUNNER_H_
#define MIR_CLIENT_WAYLAND_RUNNER_H_

#include <wayland-client.h>

namespace mir
{
namespace client
{

class WaylandRunner
{
public:
    static void run(wl_display* display);
    static void quit();
private:
    static WaylandRunner instance;
    void quit_();
    void operator()(wl_display* display);
    WaylandRunner();
};

} // mir
} // client

#endif //MIR_CLIENT_WAYLAND_RUNNER_H_
