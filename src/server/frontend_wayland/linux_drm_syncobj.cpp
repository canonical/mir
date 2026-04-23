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
#include "wl_surface.h"
#include "protocol_error.h"
#include <mir/graphics/platform.h>
#include <mir/graphics/drm_syncobj.h>
#include <boost/throw_exception.hpp>
#include <drm.h>
#include <stdexcept>
#include <system_error>
#include <wayland-server.h>
#include <xf86drm.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mw = mir::wayland_rs;

using DRMProviderList = std::vector<std::shared_ptr<mg::DRMRenderingProvider>>;

class mf::syncobj::Manager : public mw::WpLinuxDrmSyncobjManagerV1Impl
{
public:
    explicit Manager(std::shared_ptr<DRMProviderList const> providers)
        : providers{std::move(providers)}
    {
    }

    auto get_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& wl_surface)
        -> std::shared_ptr<wayland_rs::WpLinuxDrmSyncobjSurfaceV1Impl> override
    {
        auto surface = WlSurface::from(&wl_surface.value());
        try
        {
            return std::make_shared<SyncTimeline>(surface);
        }
        catch (WlSurface::TimelineAlreadyAssociated const&)
        {
            throw mw::ProtocolError{
                object_id(),
                Error::surface_exists,
                "Surface already has a linux_drm_syncobj_surface_v1 associated"};
        }
    }

    auto import_timeline(int32_t fd) -> std::shared_ptr<wayland_rs::WpLinuxDrmSyncobjTimelineV1Impl> override
    {
        try
        {
            return std::make_shared<Timeline>(Fd(IntOwnedFd(fd)), *providers);
        }
        catch (std::runtime_error const&)
        {
            throw mw::ProtocolError{
                object_id(),
                Error::invalid_timeline,
                "Failed to import DRM Syncobj timeline"};
        }
    }

private:
    std::shared_ptr<DRMProviderList const> const providers;
};

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

mf::SyncTimeline::SyncTimeline(WlSurface* surface)
{
    surface->associate_sync_timeline(mw::Weak(shared_from_this()));
}

auto mf::SyncTimeline::claim_timeline() -> Points
{
    if (!acquire_point)
    {
        BOOST_THROW_EXCEPTION((
            mw::ProtocolError{
                object_id(),
                Error::no_acquire_point,
                "Buffer does not have an acquire point set"}));
    }
    if (!release_point)
    {
        BOOST_THROW_EXCEPTION((
            mw::ProtocolError{
                object_id(),
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
    return acquire_point || release_point;
}

auto mir::frontend::SyncTimeline::set_acquire_point(
    mw::Weak<mw::WpLinuxDrmSyncobjTimelineV1Impl> const& timeline, uint32_t point_hi, uint32_t point_lo) -> void
{
    auto tl = syncobj::Timeline::from(&timeline.value());
    acquire_point.emplace(SyncPoint{
        tl->syncobj(),
        static_cast<uint64_t>(point_hi) << 32 | point_lo});
}

void mf::SyncTimeline::set_release_point(
    mw::Weak<mw::WpLinuxDrmSyncobjTimelineV1Impl> const& timeline, uint32_t point_hi, uint32_t point_lo)
{
    auto tl = syncobj::Timeline::from(&timeline.value());
    release_point.emplace(SyncPoint{
        tl->syncobj(),
        static_cast<uint64_t>(point_hi) << 32 | point_lo});
}

mf::syncobj::Timeline::Timeline(
    Fd fd,
    DRMProviderList const& providers)
{
    for (auto const& provider : providers)
    {
        if (auto obj = provider->import_syncobj(fd))
        {
            imported_timeline = std::move(obj);
        }
    }
}

auto mf::syncobj::Timeline::from(WpLinuxDrmSyncobjTimelineV1Impl* resource) -> Timeline*
{
    return dynamic_cast<Timeline*>(resource);
}

auto mf::syncobj::Timeline::syncobj() const -> std::shared_ptr<mg::drm::Syncobj>
{
    return imported_timeline;
}
