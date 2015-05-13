/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#ifndef MIR_SCENE_BUFFER_ACCESS_H_
#define MIR_SCENE_BUFFER_ACCESS_H_

#include <functional>

namespace mir
{
namespace graphics
{
class Buffer;
}
namespace scene
{

class BufferAccess
{
public:
    virtual ~BufferAccess() = default;

    virtual void with_most_recent_buffer_do(
        std::function<void(graphics::Buffer&)> const& exec) = 0;

protected:
    BufferAccess() = default;
    BufferAccess(BufferAccess const&) = delete;
    BufferAccess& operator=(BufferAccess const&) = delete;
};

}
}

#endif /* MIR_SCENE_BUFFER_ACCESS_H_ */
