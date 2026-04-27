/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_FRONTEND_EXT_IMAGE_CAPTURE_V1_H
#define MIR_FRONTEND_EXT_IMAGE_CAPTURE_V1_H

#include "ext_image_capture_source_v1.h"
#include "ext_image_copy_capture_v1.h"

#include <memory>

namespace mir
{
class Executor;
namespace compositor
{
class ScreenShooterFactory;
}
namespace input
{
class CursorObserverMultiplexer;
}
namespace time
{
class Clock;
}
namespace frontend
{
class SurfaceStack;

struct ExtImageCaptureV1Ctx;

class ExtOutputImageCaptureSourceManagerV1
    : public wayland_rs::ExtOutputImageCaptureSourceManagerV1Impl
{
public:
    ExtOutputImageCaptureSourceManagerV1(
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<compositor::ScreenShooterFactory> const& screen_shooter_factory,
        std::shared_ptr<SurfaceStack> const& surface_stack);
    auto create_source(wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output) -> std::shared_ptr<wayland_rs::ExtImageCaptureSourceV1Impl> override;

    std::shared_ptr<ExtImageCaptureV1Ctx> ctx;
};

class ExtImageCopyCaptureManagerV1
    : public wayland_rs::ExtImageCopyCaptureManagerV1Impl
{
public:
    ExtImageCopyCaptureManagerV1(
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
        std::shared_ptr<time::Clock> const& clock);

    auto create_session(wayland_rs::Weak<wayland_rs::ExtImageCaptureSourceV1Impl> const& source, uint32_t options)
        -> std::shared_ptr<wayland_rs::ExtImageCopyCaptureSessionV1Impl> override;
    auto create_pointer_cursor_session(wayland_rs::Weak<wayland_rs::ExtImageCaptureSourceV1Impl> const& source, wayland_rs::Weak<wayland_rs::WlPointerImpl> const& pointer)
        -> std::shared_ptr<wayland_rs::ExtImageCopyCaptureCursorSessionV1Impl> override;

private:
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<input::CursorObserverMultiplexer> const cursor_observer_multiplexer;
    std::shared_ptr<time::Clock> const clock;
};

}
}

#endif // MIR_FRONTEND_EXT_IMAGE_CAPTURE_V1_H
