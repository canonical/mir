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
#include "wayland.h"
#include "weak.h"
#include "client.h"
#include "protocol_error.h"
#include <mir/graphics/platform.h>
#include <mir/graphics/drm_syncobj.h>
#include <mir/fd.h>
#include "wl_surface.h"
#include <boost/throw_exception.hpp>
#include <drm.h>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <xf86drm.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mw = mir::wayland;

using DRMProviderList = std::vector<std::shared_ptr<mg::DRMRenderingProvider>>;

mf::LinuxDRMSyncobjManager::LinuxDRMSyncobjManager(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::LinuxDrmSyncobjManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<DRMProviderList const> providers)
    : mw::LinuxDrmSyncobjManagerV1{std::move(client), std::move(instance), object_id},
      providers{std::move(providers)}
{
}

auto mf::LinuxDRMSyncobjManager::get_surface(
    mw::Weak<mw::Surface> const& surface,
    rust::Box<mw::LinuxDrmSyncobjSurfaceV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::LinuxDrmSyncobjSurfaceV1>
{
    auto surf = mw::Surface::from<WlSurface>(surface);
    try
    {
        return std::make_shared<SyncTimeline>(client, std::move(child_instance), child_object_id, surf);
    }
    catch (WlSurface::TimelineAlreadyAssociated const&)
    {
        throw mw::ProtocolError{
            object_id(),
            Error::surface_exists,
            "Surface already has a linux_drm_syncobj_surface_v1 associated"};
    }
}

auto mf::LinuxDRMSyncobjManager::import_timeline(
    int32_t fd,
    rust::Box<mw::LinuxDrmSyncobjTimelineV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::LinuxDrmSyncobjTimelineV1>
{
    // The fd is borrowed across the FFI boundary; dup it so we own our copy.
    mir::Fd owned_fd{::dup(fd)};
    try
    {
        return std::make_shared<syncobj::Timeline>(client, std::move(child_instance), child_object_id, std::move(owned_fd), *providers);
    }
    catch (std::runtime_error const&)
    {
        throw mw::ProtocolError{
            object_id(),
            Error::invalid_timeline,
            "Failed to import DRM Syncobj timeline"};
    }
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

mf::SyncTimeline::SyncTimeline(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::LinuxDrmSyncobjSurfaceV1Middleware> instance,
    uint32_t object_id,
    WlSurface* surface)
    : mw::LinuxDrmSyncobjSurfaceV1{std::move(client), std::move(instance), object_id}
{
    surface->associate_sync_timeline(mw::make_weak(this));
}

auto mf::SyncTimeline::claim_timeline() -> Points
{
    if (!acquire_point)
    {
        throw mw::ProtocolError{
            object_id(),
            Error::no_acquire_point,
            "Buffer does not have an acquire point set"};
    }
    if (!release_point)
    {
        throw mw::ProtocolError{
            object_id(),
            Error::no_release_point,
            "Buffer does not have a release point set"};
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

void mf::SyncTimeline::set_acquire_point(mw::Weak<mw::LinuxDrmSyncobjTimelineV1> const& timeline, uint32_t point_hi, uint32_t point_lo)
{
    auto tl = mw::LinuxDrmSyncobjTimelineV1::from<syncobj::Timeline>(timeline);
    acquire_point.emplace(SyncPoint{
        tl->syncobj(),
        static_cast<uint64_t>(point_hi) << 32 | point_lo});
}

void mf::SyncTimeline::set_release_point(mw::Weak<mw::LinuxDrmSyncobjTimelineV1> const& timeline, uint32_t point_hi, uint32_t point_lo)
{
    auto tl = mw::LinuxDrmSyncobjTimelineV1::from<syncobj::Timeline>(timeline);
    release_point.emplace(SyncPoint{
        tl->syncobj(),
        static_cast<uint64_t>(point_hi) << 32 | point_lo});
}

mf::syncobj::Timeline::Timeline(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::LinuxDrmSyncobjTimelineV1Middleware> instance,
    uint32_t object_id,
    Fd fd,
    DRMProviderList const& providers)
    : mw::LinuxDrmSyncobjTimelineV1{std::move(client), std::move(instance), object_id}
{
    for (auto const& provider : providers)
    {
        if (auto obj = provider->import_syncobj(fd))
        {
            imported_timeline = std::move(obj);
        }
    }

    if (!imported_timeline)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("No DRM rendering provider could import the syncobj timeline"));
    }
}

auto mf::syncobj::Timeline::syncobj() const -> std::shared_ptr<mg::drm::Syncobj>
{
    return imported_timeline;
}
