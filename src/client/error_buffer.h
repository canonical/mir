/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_CLIENT_ERROR_BUFFER_H
#define MIR_CLIENT_ERROR_BUFFER_H

#include "mir/mir_buffer.h"

#include <string>

namespace mir
{
namespace client
{
class ErrorBuffer : public MirBuffer
{
public:
    ErrorBuffer(
        std::string const& error_msg, int buffer_id,
        MirBufferCallback cb, void* context, MirConnection* connection);

    int rpc_id() const override;
    void submitted() override;
    void received() override;
    void received(MirBufferPackage const& update_message) override;
    std::shared_ptr<ClientBuffer> client_buffer() const override;
    MirGraphicsRegion map_region() override;
    void unmap_region() override;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirBufferUsage buffer_usage() const override;
#pragma GCC diagnostic pop
    MirPixelFormat pixel_format() const override;
    geometry::Size size() const override;
    MirConnection* allocating_connection() const override;
    void increment_age() override;
    void set_callback(MirBufferCallback callback, void* context) override;

    bool valid() const override;
    char const* error_message() const override;
private:
    std::string const error_msg;
    int const buffer_id;
    MirBufferCallback const cb;
    void* const cb_context;
    MirConnection* connection;
};
}
}
#endif /* MIR_CLIENT_BUFFER_H_ */
