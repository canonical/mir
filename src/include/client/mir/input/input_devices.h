/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_DEVICES_H_
#define MIR_INPUT_INPUT_DEVICES_H_

#include "mir/input/mir_input_config.h"
#include "mir/client/surface_map.h"

#include <mutex>
#include <string>

namespace mir
{
namespace input
{

class InputDevices
{
public:
    InputDevices(std::shared_ptr<client::SurfaceMap> const& windows);
    void update_devices(std::string const& device_buffer);
    MirInputConfig devices();
    void set_change_callback(std::function<void()> const& callback);
private:
    std::weak_ptr<client::SurfaceMap> const windows;
    std::mutex devices_access;
    MirInputConfig configuration;
    std::function<void()> callback;
};

}
}

#endif
