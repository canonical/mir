/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include "buffer.h"
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;

mcl::Buffer::Buffer(
    mir_buffer_callback cb, void* context,
    int buffer_id,
    std::shared_ptr<ClientBuffer> const& buffer) :
    cb(cb),
    cb_context(context),
    buffer_id(buffer_id),
    buffer(buffer),
    owned(true)
{
    cb(nullptr, reinterpret_cast<MirBuffer*>(this), cb_context);
}

int mcl::Buffer::rpc_id() const
{
    return buffer_id;
}

void mcl::Buffer::submitted()
{
    if (!owned)
        BOOST_THROW_EXCEPTION(std::logic_error("cannot submit unowned buffer"));
    owned = false;
}

void mcl::Buffer::received()
{
    if (!owned)
    {
        owned = true;
        cb(nullptr, reinterpret_cast<MirBuffer*>(this), cb_context);
    }
}
