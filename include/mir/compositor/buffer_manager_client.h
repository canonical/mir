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

#ifndef MIR_COMPOSITOR_BUFFER_MANAGER_CLIENT_H_
#define MIR_COMPOSITOR_BUFFER_MANAGER_CLIENT_H_

#include "mir/compositor/buffer.h"

namespace mir
{
namespace compositor
{

class BufferManagerClient
{
public:
    BufferManagerClient();
    void add_buffer(Buffer *buffer);
    void bind_back_buffer(); 
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_MANAGER_CLIENT_H_ */
