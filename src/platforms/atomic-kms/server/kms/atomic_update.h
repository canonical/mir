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

#ifndef MIR_ATOMIC_KMS_ATOMIC_UPDATE_H_
#define MIR_ATOMIC_KMS_ATOMIC_UPDATE_H_

#include <xf86drmMode.h>

namespace mir::graphics::kms
{
class ObjectProperties;
}

namespace mir::graphics::atomic
{
class AtomicUpdate
{
public:
    AtomicUpdate();
    ~AtomicUpdate();

    AtomicUpdate(AtomicUpdate const&) = delete;
    AtomicUpdate& operator=(AtomicUpdate const&) = delete;
    AtomicUpdate(AtomicUpdate&&) = delete;
    AtomicUpdate& operator=(AtomicUpdate&&) = delete;

    void add_property(
        mir::graphics::kms::ObjectProperties const& properties,
        char const* property_name,
        uint64_t value);

    /// \return 0 on success, or a negative errno on failure
    auto commit(int fd, uint32_t flags) -> int;

private:
    drmModeAtomicReqPtr const req;
};
}

#endif
