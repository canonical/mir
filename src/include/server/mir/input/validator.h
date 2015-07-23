/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_VALIDATOR_H_
#define MIR_INPUT_VALIDATOR_H_

#include "mir/events/event_builders.h"

#include <memory>
#include <unordered_map>
#include <mutex>

namespace mir
{
namespace input
{
class Validator
{
public:
    Validator(std::function<void(MirEvent const&)> const& dispatch_valid_event);

    void validate_and_dispatch(MirEvent const& event);
    
private:
    std::mutex state_guard;

    std::function<void(MirEvent const&)> const dispatch_valid_event;

    std::unordered_map<MirInputDeviceId, EventUPtr> last_event_by_device;

    void handle_touch_event(MirInputDeviceId,
        MirTouchEvent const* event);
    void ensure_stream_validity_locked(std::lock_guard<std::mutex> const& lg,
        MirTouchEvent const* ev, MirTouchEvent const* last_ev);
};

}
}

#endif // MIR_INPUT_VALIDATOR_H_
