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

#include <mir/geometry/rectangles.h>
#include <mir/time/types.h>
#include <mir/wayland/weak.h>
#include "ext-image-capture-source-v1_wrapper.h"
#include "ext-image-copy-capture-v1_wrapper.h"

#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>

namespace mir
{
class Executor;
namespace graphics { class Buffer; }
namespace input { class CursorObserverMultiplexer; }
namespace renderer::software { class WriteMappable; }
namespace time { class Clock; }
namespace frontend
{
class ExtImageCopyCaptureSessionV1;

/// An session for the [ExtImageCopyBackend] which gets informed when it should take
/// some action, such as capturing a frame, updating its buffer constraints, or
/// stopping.
class ExtImageCopyBackendSession
{
public:
    virtual ~ExtImageCopyBackendSession() = default;
    virtual void maybe_capture_frame() = 0;
    virtual void set_buffer_constraints(geometry::Size const& size) = 0;
    virtual void set_stopped() = 0;
};

class ExtImageCopyBackend
{
public:
    using CaptureResult = std::expected<std::tuple<time::Timestamp, geometry::Rectangle>, uint32_t>;
    using CaptureCallback = std::function<void(CaptureResult const&)>;

    ExtImageCopyBackend(ExtImageCopyBackendSession* session, bool overlay_cursor);
    virtual ~ExtImageCopyBackend() = default;

    ExtImageCopyBackend(ExtImageCopyBackend const&) = delete;
    ExtImageCopyBackend& operator=(ExtImageCopyBackend const&) = delete;

    virtual bool has_damage();

    /// Acquire the source's current content without copying it.
    ///
    /// \param consumer_id identifies the consumer to the source's buffer stream for release
    ///                    bookkeeping and must be stable for the lifetime of that consumer.
    ///
    /// \returns `nullptr` when the source cannot be represented by a single existing buffer, in
    ///          which case the caller must composite the source into a buffer of its own via
    ///          [begin_capture()].
    virtual auto acquire_content(void const* consumer_id) -> std::shared_ptr<graphics::Buffer> = 0;

    /// Begin the capture.
    /// 
    /// The \p callback is executed from the Wayland thread, or immediately in the
    /// event of a failure.
    /// 
    /// \pre has_damage() == true
    virtual void begin_capture(
        std::shared_ptr<renderer::software::WriteMappable> const& shm_data,
        geometry::Rectangle const& frame_damage,
        CaptureCallback const& callback) = 0;

protected:
    enum class DamageAmount
    {
        none,
        partial,
        full,
    };

    ExtImageCopyBackendSession* const session;
    bool const overlay_cursor;

    geometry::Rectangle output_space_area;
    geometry::Rectangle output_space_damage;
    DamageAmount damage_amount = DamageAmount::full;

    void apply_damage(std::optional<geometry::Rectangle> const& damage);
};

using ExtImageCopyBackendFactory =
    std::function<std::shared_ptr<ExtImageCopyBackend>(ExtImageCopyBackendSession*, bool)>;
using ExtImageCopyCursorMapPosition = std::function<std::optional<geometry::Point>(float abs_x, float abs_y)>;

class ExtImageCaptureSourceV1 : public wayland::ImageCaptureSourceV1
{
public:
    ExtImageCaptureSourceV1(
        wl_resource* resource,
        ExtImageCopyBackendFactory const& backend_factory,
        ExtImageCopyCursorMapPosition const& cursor_map_position);

    static ExtImageCaptureSourceV1* from_or_throw(wl_resource* resource);

    ExtImageCopyBackendFactory const backend_factory;
    ExtImageCopyCursorMapPosition const cursor_map_position;
};

class ExtImageCopyCaptureFrameV1;

class ExtImageCopyCaptureSessionV1 : public wayland::ImageCopyCaptureSessionV1, public ExtImageCopyBackendSession
{
public:
    ExtImageCopyCaptureSessionV1(
        wl_resource* resource,
        bool overlay_cursor,
        ExtImageCopyBackendFactory const& backend_factory);
    ~ExtImageCopyCaptureSessionV1();

    void set_buffer_constraints(geometry::Size const& buffer_size) override;
    void set_stopped() override;
    void maybe_capture_frame() override;

private:
    void create_frame(wl_resource* new_resource) override;

    bool stopped = false;
    wayland::Weak<ExtImageCopyCaptureFrameV1> current_frame;

    std::shared_ptr<ExtImageCopyBackend> backend;
};

auto create_ext_image_copy_capture_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock) -> std::shared_ptr<wayland::ImageCopyCaptureManagerV1::Global>;

}
}

#endif // MIR_FRONTEND_EXT_IMAGE_CAPTURE_V1_H
