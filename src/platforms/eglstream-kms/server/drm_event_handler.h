/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_EGLSTREAM_DRM_EVENT_HANDLER_H_
#define MIR_PLATFORM_EGLSTREAM_DRM_EVENT_HANDLER_H_

#include <future>

namespace mir
{
namespace graphics
{
namespace eglstream
{
class DRMEventHandler
{
public:
    DRMEventHandler() = default;
    virtual ~DRMEventHandler() = default;

    DRMEventHandler(DRMEventHandler const&) = delete;
    DRMEventHandler& operator=(DRMEventHandler const&) = delete;

    virtual void const* drm_event_data() const = 0;

    using KMSCrtcId = uint32_t;
    virtual std::future<void> expect_flip_event(
        KMSCrtcId id,
        std::function<void(unsigned int frame_number, std::chrono::milliseconds frame_time)> on_flip) = 0;
};
}
}
}

#endif //MIR_PLATFORM_EGLSTREAM_DRM_EVENT_HANDLER_H_
