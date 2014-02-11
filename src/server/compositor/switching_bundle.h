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
#include <iosfwd>

#include <boost/optional/optional.hpp>

namespace mir
{
namespace graphics
{
class Buffer;
class GraphicBufferAllocator;
}
namespace compositor
{

class SwitchingBundle : public BufferBundle
{
public:
    enum {min_buffers = 1, max_buffers = 5};

    SwitchingBundle(int nbuffers,
                    const std::shared_ptr<graphics::GraphicBufferAllocator> &,
                    const graphics::BufferProperties &);

    graphics::BufferProperties properties() const;

    void client_acquire(std::function<void(graphics::Buffer* buffer)> complete) override;
    void client_release(graphics::Buffer* buffer);
    std::shared_ptr<graphics::Buffer>
        compositor_acquire(unsigned long frameno) override;
    void compositor_release(std::shared_ptr<graphics::Buffer> const& released_buffer);
    std::shared_ptr<graphics::Buffer> snapshot_acquire();
    void snapshot_release(std::shared_ptr<graphics::Buffer> const& released_buffer);
    void force_requests_to_complete();
    void allow_framedropping(bool dropping_allowed);
    bool framedropping_allowed() const;

    /**
     * Change the dimensions of the buffers contained in the bundle. For
     * client_acquire, the change is guaranteed to be immediate. For
     * compositors and snapshotters however, the change may be delayed by
     * nbuffers frames while old frames of the old size are consumed.
     */
    void resize(const geometry::Size &newsize) override;

private:
    graphics::BufferProperties bundle_properties;
    std::shared_ptr<graphics::GraphicBufferAllocator> gralloc;

    int drop_frames(int max);
    int nfree() const;
    int first_free() const;
    int next(int slot) const;
    int prev(int slot) const;
    int last_compositor() const;

    const std::shared_ptr<graphics::Buffer> &alloc_buffer(int slot);

    struct SharedBuffer
    {
        std::shared_ptr<graphics::Buffer> buf;
        int users = 0; // presently just a count of compositors sharing the buf
    };
    SharedBuffer ring[max_buffers];

    const int nbuffers;
    int first_compositor;
    int ncompositors;
    int first_ready;
    int nready;
    int first_client;
    int nclients;
    int snapshot;
    int nsnapshotters;

    mutable std::mutex guard;
    std::condition_variable cond;

    boost::optional<unsigned long> last_consumed;

    bool overlapping_compositors;

    bool framedropping;
    int force_drop;

    friend std::ostream& operator<<(std::ostream& os, const SwitchingBundle& bundle);
};

// for use when debugging. e.g "std::cout << *this << std::endl;"
std::ostream& operator<<(std::ostream& os, const SwitchingBundle& bundle);

}
}

#endif /* MIR_COMPOSITOR_SWITCHING_BUNDLE_H_ */
