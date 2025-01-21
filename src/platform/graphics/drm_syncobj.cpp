#include "mir/graphics/drm_syncobj.h"

#include "mir/log.h"

#include <boost/throw_exception.hpp>
#include <drm.h>
#include <sys/eventfd.h>
#include <system_error>
#include <xf86drm.h>

namespace mg = mir::graphics;
namespace mt = mir::time;

mg::drm::Syncobj::Syncobj(Fd drm_fd, uint32_t handle)
    : drm_fd{std::move(drm_fd)},
      handle{handle}
{
}

mg::drm::Syncobj::~Syncobj()
{
    drmSyncobjDestroy(drm_fd, handle);
}

auto mg::drm::Syncobj::wait(uint64_t point, mt::PosixClock<CLOCK_MONOTONIC>::time_point until) const -> bool
{
    if (auto err = drmSyncobjTimelineWait(
        drm_fd,
        const_cast<uint32_t *>(&handle),
        &point,
        1,
        duration_cast<std::chrono::nanoseconds>(until.time_since_epoch()).count(),
        DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT, nullptr))
    {
        if (err == -ETIME)
        {
            // Timeout; this is not an error
            return false;
        }
        BOOST_THROW_EXCEPTION((
            std::system_error{
                -err,
                std::system_category(),
                "Failed to wait for DRM sync timeline"
            }
        ));
    }
    return true;
}

void mg::drm::Syncobj::signal(uint64_t point)
{
    if (auto err = drmSyncobjTimelineSignal(
        drm_fd,
        &handle,
        &point,
        1))
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                -err,
                std::system_category(),
                "Failed to signal DRM timeline syncpoint"
            }
        ));
    }
}

auto mg::drm::Syncobj::to_eventfd(uint64_t point) const -> Fd
{
    mir::Fd event_fd{eventfd(0, EFD_CLOEXEC)};
    if (event_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to create eventfd"}));
    }
    if (auto err = drmSyncobjEventfd(drm_fd, handle, point, event_fd, 0))
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                -err,
                std::system_category(),
                "Failed to associate eventfd with DRM sync point"}));
    }
    return event_fd;
}
