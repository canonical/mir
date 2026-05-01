/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_SERVER_FRONTEND_LINUX_DRM_SYNCOBJ_H_
#define MIR_SERVER_FRONTEND_LINUX_DRM_SYNCOBJ_H_

#include "linux_drm_syncobj_v1.h"
#include <mir/graphics/platform.h>
#include <mir/time/posix_clock.h>

namespace mir::frontend
{
class WlSurface;

class LinuxDRMSyncobjManagerGlobal
{
public:
    LinuxDRMSyncobjManagerGlobal(std::span<std::shared_ptr<graphics::DRMRenderingProvider>> providers);
    auto create() -> std::shared_ptr<wayland_rs::WpLinuxDrmSyncobjManagerV1Impl>;

private:
    std::shared_ptr<std::vector<std::shared_ptr<graphics::DRMRenderingProvider>> const> const providers;
};

class SyncPoint
{
public:
    SyncPoint(std::shared_ptr<graphics::drm::Syncobj> timeline, uint64_t point);

    auto wait(time::PosixClock<CLOCK_MONOTONIC>::time_point until) const -> bool;
    void signal();
    auto to_eventfd() const -> Fd;

private:
    std::shared_ptr<graphics::drm::Syncobj> timeline;
    uint64_t point;
};

class SyncTimeline : public wayland_rs::WpLinuxDrmSyncobjSurfaceV1Impl, std::enable_shared_from_this<SyncTimeline>
{
public:
    explicit SyncTimeline(WlSurface* surface);

    struct Points
    {
        SyncPoint acquire;
        SyncPoint release;
    };
    /**
     * Acquire the current timeline, resetting to the empty state
     *
     * \throws The appropriate ProtocolError if either of acquire_point or release_point are missing
     */
     auto claim_timeline() -> Points;

    /**
     * Check if *any* timeline values are set
     *
     * This does not guarantee that both acquire and release are set, just that at least one
     * is set.
     */
    bool timeline_set() const;

    auto set_acquire_point(wayland_rs::Weak<wayland_rs::WpLinuxDrmSyncobjTimelineV1Impl> const& timeline, uint32_t point_hi, uint32_t point_lo) -> void override;
    auto set_release_point(wayland_rs::Weak<wayland_rs::WpLinuxDrmSyncobjTimelineV1Impl> const& timeline, uint32_t point_hi, uint32_t point_lo) -> void override;

private:
    std::optional<SyncPoint> acquire_point;
    std::optional<SyncPoint> release_point;
};

namespace syncobj
{
class Manager;

class Timeline : public wayland_rs::WpLinuxDrmSyncobjTimelineV1Impl
{
public:
    Timeline(
        mir::Fd fd,
        std::vector<std::shared_ptr<graphics::DRMRenderingProvider>> const& providers);
    auto syncobj() const -> std::shared_ptr<graphics::drm::Syncobj>;

    static auto from(WpLinuxDrmSyncobjTimelineV1Impl* resource) -> Timeline*;
private:
    friend class Manager;


    std::shared_ptr<graphics::drm::Syncobj> imported_timeline;
};
}
}

#endif // MIR_SERVER_LINUX_DRM_SYNCOBJ_H_
