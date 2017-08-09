/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_native_buffer.h"
#include "mir_protobuf.pb.h"
#include "protobuf_to_native_buffer.h"

std::unique_ptr<MirBufferPackage> mir::client::protobuf_to_native_buffer(
    mir::protobuf::Buffer const& buffer)
{
    auto package = std::make_unique<MirBufferPackage>();
    if (!buffer.has_error())
    {
        package->data_items = buffer.data_size();
        package->fd_items = buffer.fd_size();
        for (int i = 0; i != buffer.data_size(); ++i)
            package->data[i] = buffer.data(i);
        for (int i = 0; i != buffer.fd_size(); ++i)
            package->fd[i] = buffer.fd(i);
        package->stride = buffer.stride();
        package->flags = buffer.flags();
        package->width = buffer.width();
        package->height = buffer.height();
    }
    else
    {
        package->data_items = 0;
        package->fd_items = 0;
        package->stride = 0;
        package->flags = 0;
        package->width = 0;
        package->height = 0;
    }
    return package;
}
