/*
 * Copyright © Canonical Ltd.
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
 */


#ifndef MIR_EMERGENCY_CLEANUP_REGISTRY_H_
#define MIR_EMERGENCY_CLEANUP_REGISTRY_H_

#include <functional>
#include <mir/module_deleter.h>

namespace mir
{

using EmergencyCleanupHandler = std::function<void()>;
using ModuleEmergencyCleanupHandler = mir::UniqueModulePtr<std::function<void()>>;

class EmergencyCleanupRegistry
{
public:
    virtual ~EmergencyCleanupRegistry() = default;

    virtual void add(EmergencyCleanupHandler const& handler) = 0;
    virtual void add(ModuleEmergencyCleanupHandler handler) = 0;

protected:
    EmergencyCleanupRegistry() = default;
    EmergencyCleanupRegistry(EmergencyCleanupRegistry const&) = delete;
    EmergencyCleanupRegistry& operator=(EmergencyCleanupRegistry const&) = delete;
};

}

#endif /* MIR_EMERGENCY_CLEANUP_REGISTRY_H_ */
