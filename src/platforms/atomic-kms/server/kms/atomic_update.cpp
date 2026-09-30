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

#include <mir/graphics/kms/drm_mode_resources.h>

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mgk = mir::graphics::kms;
namespace mga = mir::graphics::atomic;

mga::AtomicUpdate::AtomicUpdate()
    : req{drmModeAtomicAlloc()}
{
    if (!req)
    {
        BOOST_THROW_EXCEPTION((
            AtomicUpdateError{"Failed to allocate Atomic DRM update request"}));
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
    drmModeAtomicAddProperty(req, properties.parent_id(), properties.id_for(property_name), value);
}
