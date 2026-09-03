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
namespace input { class CursorObserverMultiplexer; }
namespace renderer::software { class RWMappable; }
namespace time { class Clock; }
namespace frontend
{
class ExtImageCopyCaptureSessionV1;

class ExtImageCopyBackend
{
public:
    using CaptureResult = std::expected<std::tuple<time::Timestamp, geometry::Rectangle>, uint32_t>;
    using CaptureCallback = std::function<void(CaptureResult const&)>;

    ExtImageCopyBackend(ExtImageCopyCaptureSessionV1* session, bool overlay_cursor);
    virtual ~ExtImageCopyBackend() = default;

    ExtImageCopyBackend(ExtImageCopyBackend const&) = delete;
    ExtImageCopyBackend& operator=(ExtImageCopyBackend const&) = delete;

    virtual bool has_damage();

    // \pre has_damage() == true
    virtual void begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        geometry::Rectangle const& frame_damage,
        CaptureCallback const& callback) = 0;

protected:
    enum class DamageAmount
    {
        none,
        partial,
        full,
    };

    ExtImageCopyCaptureSessionV1* const session;
    bool const overlay_cursor;

    geometry::Rectangle output_space_area;
    geometry::Rectangle output_space_damage;
    DamageAmount damage_amount = DamageAmount::full;

    void apply_damage(std::optional<geometry::Rectangle> const& damage);
};

using ExtImageCopyBackendFactory =
    std::function<std::shared_ptr<ExtImageCopyBackend>(ExtImageCopyCaptureSessionV1*, bool)>;
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

class ExtImageCopyCaptureSessionV1 : public wayland::ImageCopyCaptureSessionV1
{
public:
    ExtImageCopyCaptureSessionV1(
        wl_resource* resource,
        bool overlay_cursor,
        ExtImageCopyBackendFactory const& backend_factory);
    ~ExtImageCopyCaptureSessionV1();

    void set_buffer_constraints(geometry::Size const& buffer_size);
    void set_stopped();
    void maybe_capture_frame();

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
