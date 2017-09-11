/*
 * Copyright © 2016-2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_CLIENT_WINDOW_H
#define MIR_CLIENT_WINDOW_H

#include <mir_toolkit/mir_window.h>

#include <memory>

namespace mir
{
namespace client
{
/// Handle class for MirWindow - provides automatic reference counting.
class Window
{
public:
    Window() = default;
    explicit Window(MirWindow* spec) : self{spec, deleter} {}


    operator MirWindow*() const { return self.get(); }

    void reset() { self.reset(); }

private:
    static void deleter(MirWindow* window) { mir_window_release_sync(window); }
    std::shared_ptr<MirWindow> self;
};

// Provide a deleted overload to avoid double release "accidents".
void mir_window_release_sync(Window const& window) = delete;
void mir_surface_release_sync(Window const& window) = delete;
}
}

#endif //MIR_CLIENT_WINDOW_H
