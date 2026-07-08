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
#include "weak.h"
#include <mir/graphics/platform.h>
#include <mir/time/posix_clock.h>
#include <memory>
#include <optional>
#include <vector>

namespace mir::frontend
{
class WlSurface;

class LinuxDRMSyncobjManager : public wayland_rs::LinuxDrmSyncobjManagerV1
{
public:
    LinuxDRMSyncobjManager(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::LinuxDrmSyncobjManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<std::vector<std::shared_ptr<graphics::DRMRenderingProvider>> const> providers);

private:
    auto get_surface(
        wayland_rs::Weak<wayland_rs::Surface> const& surface,
        rust::Box<wayland_rs::LinuxDrmSyncobjSurfaceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::LinuxDrmSyncobjSurfaceV1> override;

    auto import_timeline(
        int32_t fd,
        rust::Box<wayland_rs::LinuxDrmSyncobjTimelineV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::LinuxDrmSyncobjTimelineV1> override;

    std::shared_ptr<std::vector<std::shared_ptr<graphics::DRMRenderingProvider>> const> const providers;
};

class SyncPoint
{
public:
    auto wait(time::PosixClock<CLOCK_MONOTONIC>::time_point until) const -> bool;

    void signal();

    auto to_eventfd() const -> Fd;
private:
    friend class SyncTimeline;
    SyncPoint(std::shared_ptr<graphics::drm::Syncobj> timeline, uint64_t point);

    std::shared_ptr<graphics::drm::Syncobj> timeline;
    uint64_t point;
};

class SyncTimeline : public wayland_rs::LinuxDrmSyncobjSurfaceV1
{
public:
    SyncTimeline(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::LinuxDrmSyncobjSurfaceV1Middleware> instance,
        uint32_t object_id,
        WlSurface* surface);

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

private:
    void set_acquire_point(wayland_rs::Weak<wayland_rs::LinuxDrmSyncobjTimelineV1> const& timeline, uint32_t point_hi, uint32_t point_lo) override;
    void set_release_point(wayland_rs::Weak<wayland_rs::LinuxDrmSyncobjTimelineV1> const& timeline, uint32_t point_hi, uint32_t point_lo) override;

    std::optional<SyncPoint> acquire_point;
    std::optional<SyncPoint> release_point;
};

namespace syncobj
{
class Timeline : public wayland_rs::LinuxDrmSyncobjTimelineV1
{
public:
    Timeline(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::LinuxDrmSyncobjTimelineV1Middleware> instance,
        uint32_t object_id,
        mir::Fd fd,
        std::vector<std::shared_ptr<graphics::DRMRenderingProvider>> const& providers);

    auto syncobj() const -> std::shared_ptr<graphics::drm::Syncobj>;

private:
    std::shared_ptr<graphics::drm::Syncobj> imported_timeline;
};
}
}

#endif // MIR_SERVER_LINUX_DRM_SYNCOBJ_H_
