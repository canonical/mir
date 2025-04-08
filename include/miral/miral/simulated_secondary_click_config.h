/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_SIMULATED_SECONDARY_CLICK_CONFIG
#define MIRAL_SIMULATED_SECONDARY_CLICK_CONFIG

#include <chrono>
#include <memory>

namespace mir
{
class Server;
}

namespace miral
{
class SimulatedSecondaryClickConfig
{
public:
    SimulatedSecondaryClickConfig(bool enabled_by_default);

    void operator()(mir::Server& server) const;

    void enabled(bool enabled) const;
    void hold_duration(std::chrono::milliseconds hold_duration) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_SIMULATED_SECONDARY_CLICK_CONFIG
