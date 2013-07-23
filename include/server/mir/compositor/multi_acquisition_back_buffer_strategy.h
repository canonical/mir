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

#ifndef MIR_COMPOSITOR_MULTI_ACQUISTION_BACK_BUFFER_STRATEGY_H_
#define MIR_COMPOSITOR_MULTI_ACQUISTION_BACK_BUFFER_STRATEGY_H_

#include "mir/compositor/back_buffer_strategy.h"

#include <memory>
#include <vector>
#include <mutex>

namespace mir
{
namespace compositor
{
class BufferBundle;

namespace detail
{

struct AcquiredBufferInfo
{
    std::weak_ptr<graphics::Buffer> buffer;
    bool partly_released;
    int use_count;
};

}

class MultiAcquisitionBackBufferStrategy : public BackBufferStrategy
{
public:
    MultiAcquisitionBackBufferStrategy(std::shared_ptr<BufferBundle> const& buffer_bundle);

    std::shared_ptr<graphics::Buffer> acquire();
    void release(std::shared_ptr<graphics::Buffer> const& buffer);

private:
    std::vector<detail::AcquiredBufferInfo>::iterator find_info(
        std::shared_ptr<graphics::Buffer> const& buffer);

    std::shared_ptr<BufferBundle> const buffer_bundle;
    std::vector<detail::AcquiredBufferInfo> acquired_buffers;
    std::mutex mutex;
};

}
}

#endif /* MIR_COMPOSITOR_MULTI_ACQUISTION_BACK_BUFFER_STRATEGY_H_ */
