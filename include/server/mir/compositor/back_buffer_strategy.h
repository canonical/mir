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

#ifndef MIR_COMPOSITOR_BACK_BUFFER_STRATEGY_H_
#define MIR_COMPOSITOR_BACK_BUFFER_STRATEGY_H_

#include <memory>

namespace mir
{
namespace graphics { class Buffer; }

namespace compositor
{
class BackBufferStrategy
{
public:
    virtual ~BackBufferStrategy() = default;

    virtual std::shared_ptr<graphics::Buffer> acquire() = 0;
    virtual void release(std::shared_ptr<graphics::Buffer> const& buffer) = 0;

protected:
    BackBufferStrategy() = default;
    BackBufferStrategy(BackBufferStrategy const&) = delete;
    BackBufferStrategy& operator=(BackBufferStrategy const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_BACK_BUFFER_STRATEGY_H_ */
