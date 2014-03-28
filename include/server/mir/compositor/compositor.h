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

#include <mir/graphics/cursor.h>
#include <memory>

namespace mir
{
namespace compositor
{

class Compositor
{
public:
    virtual ~Compositor() {}

    virtual void start() = 0;
    virtual void stop() = 0;

    // Optional functions you may choose to implement:
    virtual std::weak_ptr<graphics::Cursor> cursor() const
        { return std::weak_ptr<graphics::Cursor>(); }
    virtual void zoom(float) {}

protected:
    Compositor() = default;
    Compositor(Compositor const&) = delete;
    Compositor& operator=(Compositor const&) = delete;
};

}
}

#endif // MIR_COMPOSITOR_COMPOSITOR_H_
