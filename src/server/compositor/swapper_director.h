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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_SWAPPER_DIRECTOR_H_
#define MIR_COMPOSITOR_SWAPPER_DIRECTOR_H_

#include "mir/compositor/buffer_properties.h"

namespace mir
{
namespace compositor
{
class Buffer;

class SwapperDirector
{
public:
    virtual ~SwapperDirector() noexcept {}
    virtual std::shared_ptr<Buffer> client_acquire() = 0;
    virtual void client_release(std::shared_ptr<Buffer> const&) = 0;
    virtual std::shared_ptr<Buffer> compositor_acquire() = 0;
    virtual void compositor_release(std::shared_ptr<Buffer> const&) = 0;

    virtual BufferProperties properties() const = 0;
    virtual void allow_framedropping(bool dropping_allowed) = 0;
    virtual void shutdown() = 0;
protected:
    SwapperDirector() = default;
    SwapperDirector(SwapperDirector const&) = delete;
    SwapperDirector& operator=(SwapperDirector const&) = delete;
};
}
}

#endif /* MIR_COMPOSITOR_SWAPPER_DIRECTOR_H_ */
