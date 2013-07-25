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

#ifndef MIR_COMPOSITOR_SWITCHING_BUNDLE_H_
#define MIR_COMPOSITOR_SWITCHING_BUNDLE_H_

#include "buffer_bundle.h"
#include <condition_variable>
#include <mutex>
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
}
namespace compositor
{
class GraphicBufferAllocator;

class SwitchingBundle : public BufferBundle 
{
public:
    SwitchingBundle(int nbuffers,
                    const std::shared_ptr<GraphicBufferAllocator> &,
                    const BufferProperties &);

    ~SwitchingBundle() noexcept;

    BufferProperties properties() const;

    std::shared_ptr<graphics::Buffer> client_acquire();
    void client_release(std::shared_ptr<graphics::Buffer> const&);
    std::shared_ptr<graphics::Buffer> compositor_acquire();
    void compositor_release(std::shared_ptr<graphics::Buffer> const& released_buffer);
    std::shared_ptr<graphics::Buffer> snapshot_acquire();
    void snapshot_release(std::shared_ptr<graphics::Buffer> const& released_buffer);
    void force_requests_to_complete();
    void allow_framedropping(bool dropping_allowed);

private:
    BufferProperties bundle_properties; //must be before swapper
    std::shared_ptr<GraphicBufferAllocator> gralloc;

    int drop_frames(int max);
    int nfree() const;

    int nbuffers;

    struct SharedBuffer
    {
        SharedBuffer() : users{0} {}
        std::shared_ptr<graphics::Buffer> buf;
        int users;
    };
    SharedBuffer *ring;
    int first_compositor;
    int ncompositors;
    int first_ready;
    int nready;
    int first_client;
    int nclients;
    int snapshot;

    std::mutex guard;
    std::condition_variable cond;

    bool framedropping;
};

}
}

#endif /* MIR_COMPOSITOR_SWITCHING_BUNDLE_H_ */
