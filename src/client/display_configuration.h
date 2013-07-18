/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_CLIENT_DISPLAY_CONFIGURATION_H_
#define MIR_CLIENT_DISPLAY_CONFIGURATION_H_

#include "mir_toolkit/client_types.h"
#include "mir_protobuf.pb.h"

namespace mir
{
namespace client
{


void delete_config_storage(MirDisplayConfiguration& config);
void copy_mir_configuration(MirDisplayConfiguration& out, MirDisplayConfiguration const& in);

class DisplayConfiguration
{
public:
    DisplayConfiguration();
    ~DisplayConfiguration();

    operator MirDisplayConfiguration&();
    void set_from_message(protobuf::Connection& connection_msg);
private:
    MirDisplayConfiguration config;
//    std::
};
}
}

#endif /* MIR_CLIENT_DISPLAY_CONFIGURATION_H_ */
