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

#ifndef MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_DEPOSITORY_H_
#define MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_DEPOSITORY_H_

#include "../client_buffer_depository.h"

#include <stdexcept>
#include <map>

namespace mir
{
namespace client
{
class ClientBuffer;

namespace android
{
class AndroidRegistrar;

class AndroidClientBufferDepository : public ClientBufferDepository
{
public:
    AndroidClientBufferDepository(const std::shared_ptr<AndroidRegistrar>&);

    void deposit_package(std::shared_ptr<mir_toolkit::MirBufferPackage> && package, int, geometry::Size sz, geometry::PixelFormat pf);

    std::shared_ptr<ClientBuffer> access_buffer(int id);
private:
    std::shared_ptr<AndroidRegistrar> registrar;

    std::map<int, std::shared_ptr<mir::client::ClientBuffer>> buffer_depository;
};

}
}
}
#endif /* MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_DEPOSITORY_H_ */
