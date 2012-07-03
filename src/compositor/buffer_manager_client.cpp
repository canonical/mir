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
{
}

mc::BufferManagerClient::~BufferManagerClient()
{
}

void mc::BufferManagerClient::add_buffer(std::shared_ptr<Buffer>) {
    
}

int mc::BufferManagerClient::remove_all_buffers() {
    return 0;  
}

void mc::BufferManagerClient::bind_back_buffer() {

}

void mc::BufferManagerClient::dequeue_client_buffer() {

}


