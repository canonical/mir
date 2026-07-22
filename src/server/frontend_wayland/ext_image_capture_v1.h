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

#include "ext_image_capture_source_v1.h"
#include "ext_image_copy_capture_v1.h"
#include "weak.h"

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

class ExtImageCaptureSourceV1 : public wayland::ExtImageCaptureSourceV1
{
public:
    ExtImageCaptureSourceV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::ExtImageCaptureSourceV1Middleware> instance,
        uint32_t object_id,
        ExtImageCopyBackendFactory backend_factory,
        ExtImageCopyCursorMapPosition cursor_map_position);

    static auto from_or_throw(wayland::Weak<wayland::ExtImageCaptureSourceV1> const& source)
        -> ExtImageCaptureSourceV1&;

    ExtImageCopyBackendFactory const backend_factory;
    ExtImageCopyCursorMapPosition const cursor_map_position;
};

class ExtImageCopyCaptureFrameV1;

class ExtImageCopyCaptureSessionV1 : public wayland::ExtImageCopyCaptureSessionV1
{
public:
    ExtImageCopyCaptureSessionV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::ExtImageCopyCaptureSessionV1Middleware> instance,
        uint32_t object_id,
        bool overlay_cursor,
        ExtImageCopyBackendFactory const& backend_factory);
    ~ExtImageCopyCaptureSessionV1();

    void set_buffer_constraints(geometry::Size const& buffer_size);
    void set_stopped();
    void maybe_capture_frame();

private:
    auto create_frame(
        rust::Box<wayland::ExtImageCopyCaptureFrameV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::ExtImageCopyCaptureFrameV1> override;

    bool stopped = false;
    wayland::Weak<ExtImageCopyCaptureFrameV1> current_frame;

    std::shared_ptr<ExtImageCopyBackend> backend;
};

auto create_ext_image_copy_capture_manager_v1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::ExtImageCopyCaptureManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock) -> std::shared_ptr<wayland::ExtImageCopyCaptureManagerV1>;

}
}

#endif // MIR_FRONTEND_EXT_IMAGE_CAPTURE_V1_H
