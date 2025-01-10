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


#include "linux_drm_syncobj.h"
#include "linux-drm-syncobj-v1_wrapper.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/drm_syncobj.h"
#include "mir/wayland/protocol_error.h"
#include "mir/wayland/weak.h"
#include "wl_surface.h"
#include <boost/throw_exception.hpp>
#include <drm.h>
#include <stdexcept>
#include <system_error>
#include <wayland-server.h>
#include <xf86drm.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;

using DRMProviderList = std::vector<std::shared_ptr<mg::DRMRenderingProvider>>;

class mf::syncobj::Manager : public mir::wayland::LinuxDrmSyncobjManagerV1
{
public:
    Manager(
        struct wl_resource* resource,
        std::shared_ptr<DRMProviderList const> providers)
        : LinuxDrmSyncobjManagerV1(resource, Version<1>{}),
          providers{std::move(providers)}
    {
    }

private:
    void get_surface(struct wl_resource* id, struct wl_resource* wl_surface) override
    {
        auto surface = WlSurface::from(wl_surface);
        try
        {
            new SyncTimeline(id, surface);
        }
        catch (WlSurface::TimelineAlreadyAssociated const&)
        {
            throw wayland::ProtocolError{
                resource,
                Error::surface_exists,
                "Surface already has a linux_drm_syncobj_surface_v1 associated"};
        }
    }
    void import_timeline(struct wl_resource* id, mir::Fd fd) override
    {
        try
        {
            new Timeline(id, std::move(fd), *providers);
        }
        catch (std::runtime_error const&)
        {
            throw wayland::ProtocolError{
                resource,
                Error::invalid_timeline,
                "Failed to import DRM Syncobj timeline"};
        }
    }

    std::shared_ptr<DRMProviderList const> const providers;
};



mf::LinuxDRMSyncobjManager::LinuxDRMSyncobjManager(
    struct wl_display* display,
    std::span<std::shared_ptr<graphics::DRMRenderingProvider>> providers)
    : Global(display, Version<1>{}),
      providers{std::make_shared<std::vector<std::shared_ptr<graphics::DRMRenderingProvider>>>(
          providers.begin(),
          providers.end())}
{
}

void mf::LinuxDRMSyncobjManager::bind(struct wl_resource* new_resource)
{
    new syncobj::Manager{new_resource, providers};
}

mf::SyncPoint::SyncPoint(std::shared_ptr<mg::drm::Syncobj> timeline, uint64_t point)
    : timeline{std::move(timeline)},
      point{point}
{
}

void mf::SyncPoint::signal()
{
    timeline->signal(point);
}

auto mf::SyncPoint::wait(mir::time::PosixClock<CLOCK_MONOTONIC>::time_point until) const -> bool
{
    return timeline->wait(point, until);
}

auto mf::SyncPoint::to_eventfd() const -> Fd
{
    return timeline->to_eventfd(point);
}

mf::SyncTimeline::SyncTimeline(struct wl_resource* new_resource, WlSurface* surface)
    : mir::wayland::LinuxDrmSyncobjSurfaceV1(new_resource, Version<1>{})
{
    surface->associate_sync_timeline(wayland::make_weak(this));
}

auto mf::SyncTimeline::claim_timeline() -> Points
{
    if (!acquire_point)
    {
        BOOST_THROW_EXCEPTION((
            wayland::ProtocolError{
                resource,
                Error::no_acquire_point,
                "Buffer does not have an acquire point set"}));
    }
    if (!release_point)
    {
        BOOST_THROW_EXCEPTION((
            wayland::ProtocolError{
                resource,
                Error::no_release_point,
                "Buffer does not have a release point set"}));
    }

    return Points {
        .acquire = std::move(std::exchange(acquire_point, std::nullopt).value()),
        .release = std::move(std::exchange(release_point, std::nullopt).value())
    };
}

auto mf::SyncTimeline::timeline_set() const -> bool
{
    return true;
}

void mf::SyncTimeline::set_acquire_point(struct wl_resource* timeline, uint32_t point_hi, uint32_t point_lo)
{
    auto tl = syncobj::Timeline::from(timeline);
    acquire_point.emplace(SyncPoint{
        tl->syncobj(),
        static_cast<uint64_t>(point_hi) << 32 | point_lo});
}

void mf::SyncTimeline::set_release_point(struct wl_resource* timeline, uint32_t point_hi, uint32_t point_lo)
{
    auto tl = syncobj::Timeline::from(timeline);
    release_point.emplace(SyncPoint{
        tl->syncobj(),
        static_cast<uint64_t>(point_hi) << 32 | point_lo});
}

mf::syncobj::Timeline::Timeline(
    struct wl_resource* new_timeline,
    Fd fd,
    DRMProviderList const& providers)
    : LinuxDrmSyncobjTimelineV1(new_timeline, Version<1>{})
{
    for (auto const& provider : providers)
    {
        if (auto obj = provider->import_syncobj(fd))
        {
            imported_timeline = std::move(obj);
        }
    }
}

auto mf::syncobj::Timeline::from(struct wl_resource* resource) -> Timeline*
{
    return dynamic_cast<Timeline*>(LinuxDrmSyncobjTimelineV1::from(resource));
}

auto mf::syncobj::Timeline::syncobj() const -> std::shared_ptr<mg::drm::Syncobj>
{
    return imported_timeline;
}
