/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_CLIENT_BUFFER_RECEIVER_H_
#define MIR_CLIENT_BUFFER_RECEIVER_H_

namespace mir
{
namespace protobuf
{
class Buffer;
}
namespace client
{

class BufferReceiver
{
public:
    virtual void buffer_available(mir::protobuf::Buffer const& buffer) = 0;
    virtual void buffer_unavailable() = 0;
protected:
    virtual ~BufferReceiver() = default;
    BufferReceiver() = default;
    BufferReceiver(const BufferReceiver&) = delete;
    BufferReceiver& operator=(const BufferReceiver&) = delete;
};

}
}
#endif /* MIR_CLIENT_BUFFER_RECEIVER_H_ */
