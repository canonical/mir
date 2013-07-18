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

#include "mir/compositor/multi_acquisition_back_buffer_strategy.h"
#include "mir/compositor/buffer_swapper_exceptions.h"
#include "buffer_bundle.h"

#include <algorithm>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::MultiAcquisitionBackBufferStrategy::MultiAcquisitionBackBufferStrategy(
    std::shared_ptr<BufferBundle> const& buffer_bundle)
    : buffer_bundle{buffer_bundle}
{
}

std::shared_ptr<mg::Buffer> mc::MultiAcquisitionBackBufferStrategy::acquire()
{
    std::lock_guard<std::mutex> lg{mutex};

    /* Try to find an already acquired buffer, that hasn't been partly released */
    auto iter = std::find_if(acquired_buffers.begin(), acquired_buffers.end(),
                    [] (detail::AcquiredBufferInfo const& info)
                    {
                        return info.partly_released == false;
                    });

    std::shared_ptr<mg::Buffer> buffer;

    if (iter != acquired_buffers.end())
    {
        buffer = iter->buffer.lock();
    }
    else
    {
        /*
         * If we couldn't get a non-partly-released acquired buffer,
         * try to get a new one. If that fails, try to give back a partly
         * released acquired buffer.
         */
        try
        {
            buffer = buffer_bundle->compositor_acquire();
        }
        catch (mc::BufferSwapperOutOfBuffersException const& e)
        {
            if (!acquired_buffers.empty())
                buffer = acquired_buffers.back().buffer.lock();
            else
                throw;
        }

        iter = find_info(buffer);
    }


    if (iter != acquired_buffers.end())
        ++iter->use_count;
    else
        acquired_buffers.push_back(detail::AcquiredBufferInfo{buffer, false, 1});

    return buffer;
}

void mc::MultiAcquisitionBackBufferStrategy::release(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::lock_guard<std::mutex> lg{mutex};

    auto iter = find_info(buffer);

    bool should_be_released{true};

    if (iter != acquired_buffers.end())
    {
        detail::AcquiredBufferInfo& info = *iter;

        --info.use_count;

        if (info.use_count == 0)
        {
            acquired_buffers.erase(iter);
        }
        else
        {
            info.partly_released = true;
            should_be_released = false;
        }
    }

    if (should_be_released)
        buffer_bundle->compositor_release(buffer);
}

std::vector<mc::detail::AcquiredBufferInfo>::iterator
mc::MultiAcquisitionBackBufferStrategy::find_info(std::shared_ptr<mg::Buffer> const& buffer)
{
    return std::find_if(acquired_buffers.begin(), acquired_buffers.end(),
                        [&buffer] (detail::AcquiredBufferInfo const& info)
                        {
                            return info.buffer.lock() == buffer;
                        });
}
