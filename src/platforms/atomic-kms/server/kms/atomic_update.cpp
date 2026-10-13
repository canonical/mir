/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "atomic_update.h"

#include <mir/errno_utils.h>
#include <mir/fatal.h>
#include <mir/graphics/kms/drm_mode_resources.h>

namespace mgk = mir::graphics::kms;
namespace mga = mir::graphics::atomic;

mga::AtomicUpdate::AtomicUpdate()
    : req{drmModeAtomicAlloc()}
{
    if (!req)
    {
        // Only fails on memory exhaustion; there is no meaningful recovery.
        MIR_FATAL_ERROR("Failed to allocate Atomic DRM update request");
    }
}

mga::AtomicUpdate::~AtomicUpdate()
{
    drmModeAtomicFree(req);
}

auto mga::AtomicUpdate::commit(int fd, uint32_t flags) -> int
{
    return drmModeAtomicCommit(fd, req, flags, nullptr);
}

void mga::AtomicUpdate::add_property(
    mgk::ObjectProperties const& properties,
    char const* property_name,
    uint64_t value)
{
    auto const object_id = properties.parent_id();
    if (!properties.has_property(property_name))
    {
        MIR_FATAL_ERROR(
            "Missing DRM atomic property '{}' on object ID {}",
            property_name,
            object_id);
    }
    auto const property_id = properties.id_for(property_name);
    auto const result = drmModeAtomicAddProperty(req, object_id, property_id, value);
    if (result < 0)
    {
        // Only fails on programmer error or memory exhaustion; neither is recoverable.
        MIR_FATAL_ERROR(
            "Failed to add DRM atomic property '{}' (property ID {}, object ID {}): {} ({})",
            property_name,
            property_id,
            object_id,
            mir::errno_to_cstr(-result),
            -result);
    }
}
