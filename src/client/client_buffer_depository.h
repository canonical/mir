/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_
#define MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_

#include <memory>
#include "mir/geometry/pixel_format.h"
#include "mir/geometry/size.h"

namespace mir_toolkit
{
class MirBufferPackage;
}
namespace mir
{

namespace client
{
class ClientBuffer;

class ClientBufferDepository
{
public:
    virtual void deposit_package(std::shared_ptr<mir_toolkit::MirBufferPackage> &&, int id,
                                geometry::Size, geometry::PixelFormat) = 0;
    virtual std::shared_ptr<ClientBuffer> access_buffer(int id) = 0;
};
}
}
#endif /* MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_ */
