/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
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
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_ANDROID_BUFFER_FACTORY_H_
#define MIR_CLIENT_ANDROID_ANDROID_BUFFER_FACTORY_H_

#include <memory>

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"

#include "../client_buffer_factory.h"

namespace mir
{
namespace client
{
class ClientBuffer;

namespace android
{
class AndroidRegistrar;

class AndroidClientBufferFactory : public ClientBufferFactory
{
public:
    explicit AndroidClientBufferFactory(std::shared_ptr<AndroidRegistrar> const&);

    virtual std::shared_ptr<ClientBuffer> create_buffer(std::shared_ptr<MirBufferPackage> const& package,
                                                        geometry::Size size, MirPixelFormat pf);
private:
    std::shared_ptr<AndroidRegistrar> registrar;
};

}
}
}
#endif /* MIR_CLIENT_ANDROID_ANDROID_BUFFER_FACTORY_H_ */
