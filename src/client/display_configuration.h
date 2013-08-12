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

#include <mutex>
#include <functional>
#include <vector>
#include <memory>

namespace mir
{
namespace client
{
class DisplayOutput : public MirDisplayOutput
{
public:
    DisplayOutput(size_t num_modes_, size_t num_formats);
    ~DisplayOutput();
};

//convenient helper
void delete_config_storage(MirDisplayConfiguration* config);

class DisplayConfiguration
{
public:
    DisplayConfiguration();
    ~DisplayConfiguration();

    void set_configuration(mir::protobuf::DisplayConfiguration const& msg);
    void update_configuration(mir::protobuf::DisplayConfiguration const& msg);
    void set_display_change_handler(std::function<void()> const&);

    //copying to a c POD, so kinda kludgy
    MirDisplayConfiguration* copy_to_client() const;

private:
    std::mutex mutable guard;
    std::vector<std::shared_ptr<DisplayOutput>> outputs;
    std::function<void()> notify_change;
};


}
}

#endif /* MIR_CLIENT_DISPLAY_CONFIGURATION_H_ */
