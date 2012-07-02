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
#include <mir/compositor/buffer_manager_client.h>

namespace mc = mir::compositor;

mc::BufferManagerClient::BufferManagerClient()
 :
compositor_buffer(nullptr)
{
}

void mc::BufferManagerClient::add_buffer(mc::Buffer * buffer) {
    atomic_store(&compositor_buffer, buffer);
}

void mc::BufferManagerClient::bind_back_buffer() {
    if ( compositor_buffer == nullptr)
        return;

    mc::Buffer *back_buffer = nullptr;

    bool worked = false;

    back_buffer = atomic_load(&compositor_buffer);
    while (!worked) { 
        back_buffer->lock();
        back_buffer->bind_to_texture();

        worked = atomic_compare_exchange_weak(&compositor_buffer, &back_buffer, back_buffer);
        if (!worked) {
            //todo: add unbind and unlock methods and do them here 
        }
    }
}


