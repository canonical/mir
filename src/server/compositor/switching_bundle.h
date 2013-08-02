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
#include "rw_lock.h"
#include <condition_variable>
#include <memory>
#include <atomic>

namespace mir
{
namespace compositor
{
class Buffer;
class BufferSwapper;
class BufferAllocationStrategy;

class SwitchingBundle : public BufferBundle 
{
public:
    SwitchingBundle(std::shared_ptr<BufferAllocationStrategy> const& swapper_factory, graphics::BufferProperties const&);

    graphics::BufferProperties properties() const;

    std::shared_ptr<graphics::Buffer> client_acquire();
    void client_release(std::shared_ptr<graphics::Buffer> const&);
    std::shared_ptr<graphics::Buffer> compositor_acquire();
    void compositor_release(std::shared_ptr<graphics::Buffer> const& released_buffer);
    void force_requests_to_complete();
    void allow_framedropping(bool dropping_allowed);

private:
    graphics::BufferProperties bundle_properties; //must be before swapper
    std::shared_ptr<BufferAllocationStrategy> const swapper_factory;
    std::shared_ptr<BufferSwapper> swapper;
    RWLockWriterBias rw_lock;
    std::condition_variable_any cv;
};

}
}

#endif /* MIR_COMPOSITOR_SWITCHING_BUNDLE_H_ */
