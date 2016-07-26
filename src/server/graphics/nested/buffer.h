/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_BUFFER_H_
#define MIR_GRAPHICS_NESTED_BUFFER_H_

#include "mir/graphics/buffer_properties.h"
#include <memory>

namespace mir
{
namespace graphics
{
namespace nested
{
class HostConnection;
class Buffer
{
public:
    Buffer(std::shared_ptr<HostConnection> const& connection, BufferProperties const& properties);
private:
    std::shared_ptr<HostConnection> const connection;
    BufferProperties const properties;
};
}
}
}
#endif /* MIR_GRAPHICS_NESTED_BUFFERH_H_ */
