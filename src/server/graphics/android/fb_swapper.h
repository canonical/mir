/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_FB_SWAPPER_H_
#define MIR_GRAPHICS_ANDROID_FB_SWAPPER_H_

#include "mir/compositor/buffer_swapper.h"

#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>

namespace mir
{
namespace graphics
{
namespace android
{

class AndroidBuffer;
class FBSwapper // : public compositor::BufferSwapper
{
public:
    explicit FBSwapper(std::initializer_list<std::shared_ptr<AndroidBuffer>> buffer_list);
    explicit FBSwapper(std::vector<std::shared_ptr<AndroidBuffer>> buffer_list);

    std::shared_ptr<AndroidBuffer> client_acquire();
    void client_release(std::shared_ptr<AndroidBuffer> const& queued_buffer);
    std::shared_ptr<AndroidBuffer> compositor_acquire();
    void compositor_release(std::shared_ptr<AndroidBuffer> const& released_buffer);
    void shutdown();

private:
    template<class T>
    void initialize_queues(T);

    void composition_bypass_unsupported();

    std::mutex queue_lock;
    std::condition_variable cv;

    std::queue<std::shared_ptr<AndroidBuffer>> queue;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_FB_SWAPPER_H_ */
