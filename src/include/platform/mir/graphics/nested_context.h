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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_CONTEXT_H_
#define MIR_GRAPHICS_NESTED_CONTEXT_H_

#include "mir/optional_value.h"
#include "mir/fd.h"
#include <memory>
#include <vector>

struct gbm_device;
struct MirConnection;

namespace mir
{
namespace graphics
{
struct PlatformOperationMessage;

class MesaAuthExtensions
{
public:
    virtual ~MesaAuthExtensions() = default;
    virtual mir::Fd auth_fd() = 0;
protected:
    MesaAuthExtensions() = default;
    MesaAuthExtensions(MesaAuthExtensions const&) = delete;
    MesaAuthExtensions& operator=(MesaAuthExtensions const&) = delete;
};

class NestedContext
{
public:
    virtual ~NestedContext() = default;

    //unique_ptr would be nice, but don't want to break mircore
    virtual mir::optional_value<std::shared_ptr<MesaAuthExtensions>> auth_extensions() = 0;
    virtual std::vector<int> platform_fd_items() = 0;
    virtual PlatformOperationMessage platform_operation(
        unsigned int op, PlatformOperationMessage const& request) = 0;

protected:
    NestedContext() = default;
    NestedContext(NestedContext const&) = delete;
    NestedContext& operator=(NestedContext const&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_NESTED_CONTEXT_H_ */
