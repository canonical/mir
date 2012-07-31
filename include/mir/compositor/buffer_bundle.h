/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_COMPOSITOR_BUFFER_BUNDLE_H_
#define MIR_COMPOSITOR_BUFFER_BUNDLE_H_

#include "buffer_texture_binder.h"
#include "buffer_swapper.h"
#include "buffer_ipc_package.h"
#include "buffer_queue.h"
#include "buffer.h"
#include "mir/graphics/texture.h"

#include "mir/thread/all.h"

#include <memory>
#include <vector>

namespace mir
{
namespace compositor
{

class TexDeleter {

public:
    TexDeleter(std::shared_ptr<BufferSwapper> sw, std::shared_ptr<Buffer> && buf)
    : swapper(sw),
      buffer_ptr(buf)
    {
    };

    void operator()(graphics::Texture* texture)
    {
        swapper->compositor_release(buffer_ptr);
        delete texture;
    }
    
private:
    std::shared_ptr<BufferSwapper> swapper;
    std::shared_ptr<Buffer> buffer_ptr;
};


class BufDeleter {
public:
    BufDeleter(std::shared_ptr<BufferSwapper> sw, std::shared_ptr<Buffer> && buf)
    : swapper(sw),
      buffer_ptr(buf)
    {
    };

    void operator()(BufferIPCPackage* package)
    {
        swapper->client_release(buffer_ptr);
        delete package;
    }
    
private:
    std::shared_ptr<BufferSwapper> swapper;
    std::shared_ptr<Buffer> buffer_ptr;
};


class BufferBundle : public BufferTextureBinder,
    public BufferQueue
{
public:
    explicit BufferBundle(std::shared_ptr<BufferSwapper>&& swapper);
    ~BufferBundle();

    /* from BufferQueue */
    std::shared_ptr<BufferIPCPackage> secure_client_buffer();

    /* from BufferTextureBinder */
    std::shared_ptr<graphics::Texture> lock_and_bind_back_buffer();

protected:
    BufferBundle(const BufferBundle&) = delete;
    BufferBundle& operator=(const BufferBundle&) = delete;

private:
    std::shared_ptr<BufferSwapper> swapper;

};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_BUNDLE_H_ */
