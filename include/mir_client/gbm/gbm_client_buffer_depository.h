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
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_GBM_GBM_CLIENT_BUFFER_DEPOSITORY_H_
#define MIR_CLIENT_GBM_GBM_CLIENT_BUFFER_DEPOSITORY_H_

#include "mir_client/client_buffer_depository.h"

#include <stdexcept>
#include <map>

namespace mir
{
namespace client
{
class ClientBuffer;

class GBMClientBufferDepository : public ClientBufferDepository
{
public:
    GBMClientBufferDepository();

    void deposit_package(std::shared_ptr<MirBufferPackage> && package, int, geometry::Size size, geometry::PixelFormat pf);

    std::shared_ptr<ClientBuffer> access_buffer(int id);
private:
    std::shared_ptr<ClientBuffer> buffer;
};

}
}
#endif /* MIR_CLIENT_GBM_GBM_CLIENT_BUFFER_DEPOSITORY_H_ */
