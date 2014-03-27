/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_COMPOSITOR_H_
#define MIR_COMPOSITOR_COMPOSITOR_H_

#include <mir/input/cursor_listener.h>

namespace mir
{
namespace compositor
{

/*
 * Why would Compositors need to know about the cursor? Multiple reasons:
 *  1. To force screen redraw if rendering software cursors.
 *  2. To force screen redraw during desktop zoom on pointer movement.
 * Maybe there's a nicer way to deliver these events to Compositor?...
 */

class Compositor : public input::CursorListener
{
public:
    virtual ~Compositor() {}

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void cursor_moved_to(float, float) {}  // Optional
    virtual void zoom(float) {} // Optional. TODO: Generalize set(Effect)?

protected:
    Compositor() = default;
    Compositor(Compositor const&) = delete;
    Compositor& operator=(Compositor const&) = delete;
};

}
}

#endif // MIR_COMPOSITOR_COMPOSITOR_H_
