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

#ifndef MIR_COMPOSITOR_SWAPPER_SWITCHER_H_
#define MIR_COMPOSITOR_SWAPPER_SWITCHER_H_

#include "rw_lock.h"
#include <memory>

namespace mir
{
namespace compositor
{
class Buffer;
class BufferSwapper;

class SwapperSwitcher
{
public:
    SwapperSwitcher(std::shared_ptr<BufferSwapper> const& initial_swapper);

    std::shared_ptr<Buffer> client_acquire();
    void client_release(std::shared_ptr<Buffer> const& queued_buffer);
    std::shared_ptr<Buffer> compositor_acquire();
    void compositor_release(std::shared_ptr<Buffer> const& released_buffer);
    void force_requests_to_complete();

    void switch_swapper(std::shared_ptr<BufferSwapper> const& new_swapper);

private:
    std::shared_ptr<BufferSwapper> swapper;
    RWLockWriterBias rw_lock;
#if 0
protected:
    SwapperSwitcher() = default;
    SwapperSwitcher(SwapperSwitcher const&) = delete;
    SwapperSwitcher& operator=(SwapperSwitcher const&) = delete;
#endif
};

}
}

#endif /* MIR_COMPOSITOR_SWAPPER_SWITCHER_H_ */
