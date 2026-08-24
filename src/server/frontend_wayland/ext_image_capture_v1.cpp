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

#include "ext_image_capture_v1.h"

#include <glm/glm.hpp>

#include <mir/executor.h>
#include <mir/fatal.h>
#include <mir/graphics/cursor_image.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/time/clock.h>
#include "shm.h"
#include "wayland_timespec.h"
#include "protocol_error.h"

#include "wayland_rs/src/ffi.rs.h"

#include <boost/throw_exception.hpp>

#include <cstring>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace mwrs = mir::wayland;
namespace geom = mir::geometry;

namespace mir::frontend
{

/* Image capture sessions */
class ExtImageCopyCaptureManagerV1
    : public wayland::ExtImageCopyCaptureManagerV1
{
public:
    ExtImageCopyCaptureManagerV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::ExtImageCopyCaptureManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
        std::shared_ptr<time::Clock> const& clock);

private:
    auto create_session(
        wayland::Weak<wayland::ExtImageCaptureSourceV1> const& source,
        uint32_t options,
        rust::Box<wayland::ExtImageCopyCaptureSessionV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::ExtImageCopyCaptureSessionV1> override;
    auto create_pointer_cursor_session(
        wayland::Weak<wayland::ExtImageCaptureSourceV1> const& source,
        wayland::Weak<wayland::Pointer> const& pointer,
        rust::Box<wayland::ExtImageCopyCaptureCursorSessionV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::ExtImageCopyCaptureCursorSessionV1> override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<input::CursorObserverMultiplexer> const cursor_observer_multiplexer;
    std::shared_ptr<time::Clock> const clock;
};

class ExtImageCopyCaptureFrameV1
    : public wayland::ExtImageCopyCaptureFrameV1
{
public:
    ExtImageCopyCaptureFrameV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::ExtImageCopyCaptureFrameV1Middleware> instance,
        uint32_t object_id,
        ExtImageCopyCaptureSessionV1* session);

    bool is_ready() const;
    // \pre backend.has_damage() == true
    void begin_capture(ExtImageCopyBackend& backend);
    void report_result(ExtImageCopyBackend::CaptureResult const& result);

private:
    auto attach_buffer(wayland::Weak<wayland::Buffer> const& buffer) -> void override;
    auto damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) -> void override;
    auto capture() -> void override;

    bool capture_has_been_called = false;
    bool capture_has_begun = false;
    wayland::Weak<wayland::Buffer> target;
    geom::Rectangle frame_damage;

    wayland::Weak<ExtImageCopyCaptureSessionV1> const session;
};

class ExtImageCopyCaptureCursorSessionV1
    : public wayland::ExtImageCopyCaptureCursorSessionV1
{
public:
  ExtImageCopyCaptureCursorSessionV1(
      std::shared_ptr<wayland::Client> client,
      rust::Box<wayland::ExtImageCopyCaptureCursorSessionV1Middleware> instance,
      uint32_t object_id,
      std::shared_ptr<Executor> const& wayland_executor,
      std::shared_ptr<input::CursorObserverMultiplexer> const&
          cursor_observer_multiplexer,
      std::shared_ptr<time::Clock> const& clock,
      ExtImageCopyCursorMapPosition const& map_position);
  ~ExtImageCopyCaptureCursorSessionV1();

private:
    class CursorObserver;
    class ImageCopyBackend;

    auto get_capture_session(
        rust::Box<wayland::ExtImageCopyCaptureSessionV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::ExtImageCopyCaptureSessionV1> override;

    void cursor_moved_to(float abs_x, float abs_y);
    void pointer_usable();
    void pointer_unusable();

    bool pointer_is_usable = true;
    bool pointer_in_source = false;
    wayland::Weak<ExtImageCopyCaptureSessionV1> cursor_image_session;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<input::CursorObserverMultiplexer> const cursor_observer_multiplexer;
    std::shared_ptr<CursorObserver> const cursor_observer;
    std::shared_ptr<time::Clock> const clock;
    ExtImageCopyCursorMapPosition const map_position;
};

}

/* Image copy backends */

mf::ExtImageCopyBackend::ExtImageCopyBackend(
    ExtImageCopyCaptureSessionV1 *session,
    bool overlay_cursor)
    : session{session},
      overlay_cursor{overlay_cursor}
{
}

void mf::ExtImageCopyBackend::apply_damage(
    std::optional<geom::Rectangle> const& damage)
{
    if (damage && damage_amount != DamageAmount::full)
    {
        auto const intersection = intersection_of(damage.value(), output_space_area);
        if (intersection.size != geom::Size{})
        {
            if (damage_amount == DamageAmount::partial)
            {
                output_space_damage = geom::Rectangles{output_space_damage, intersection}.bounding_rectangle();
            }
            else // damage_amount == DamageAmount::none
            {
                damage_amount = DamageAmount::partial;
                output_space_damage = intersection;
            }
        }
    }
    else
    {
        // If damage amount is already full, or if damage is nullopt
        // (and thus full damage should be applied)
        damage_amount = DamageAmount::full;
    }

    if (damage_amount != DamageAmount::none)
    {
        session->maybe_capture_frame();
    }
}

bool mf::ExtImageCopyBackend::has_damage()
{
    return damage_amount != DamageAmount::none;
}

/* Image capture sources */

mf::ExtImageCaptureSourceV1::ExtImageCaptureSourceV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ExtImageCaptureSourceV1Middleware> instance,
    uint32_t object_id,
    ExtImageCopyBackendFactory backend_factory,
    ExtImageCopyCursorMapPosition cursor_map_position)
    : wayland::ExtImageCaptureSourceV1{std::move(client), std::move(instance), object_id},
      backend_factory{std::move(backend_factory)},
      cursor_map_position{std::move(cursor_map_position)}
{
}

auto mf::ExtImageCaptureSourceV1::from_or_throw(mwrs::Weak<mwrs::ExtImageCaptureSourceV1> const& source)
    -> ExtImageCaptureSourceV1&
{
    auto const instance = mwrs::ExtImageCaptureSourceV1::from<ExtImageCaptureSourceV1>(source);
    if (!instance)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "ext_image_capture_source_v1 is not a mir::frontend::ExtImageCaptureSourceV1"));
    }
    return *instance;
}

/* Image capture sessions */

auto mf::create_ext_image_copy_capture_manager_v1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ExtImageCopyCaptureManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock)
-> std::shared_ptr<mwrs::ExtImageCopyCaptureManagerV1>
{
    return std::make_shared<ExtImageCopyCaptureManagerV1>(
        std::move(client), std::move(instance), object_id, wayland_executor, cursor_observer_multiplexer, clock);
}

mf::ExtImageCopyCaptureManagerV1::ExtImageCopyCaptureManagerV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ExtImageCopyCaptureManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock)
    : wayland::ExtImageCopyCaptureManagerV1{std::move(client), std::move(instance), object_id},
      wayland_executor{wayland_executor},
      cursor_observer_multiplexer{cursor_observer_multiplexer},
      clock{clock}
{
}

auto mf::ExtImageCopyCaptureManagerV1::create_session(
    mwrs::Weak<mwrs::ExtImageCaptureSourceV1> const& source,
    uint32_t options,
    rust::Box<mwrs::ExtImageCopyCaptureSessionV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::ExtImageCopyCaptureSessionV1>
{
    auto& source_instance = ExtImageCaptureSourceV1::from_or_throw(source);
    bool overlay_cursor = (options & Options::paint_cursors) != 0;

    return std::make_shared<ExtImageCopyCaptureSessionV1>(
        client, std::move(child_instance), child_object_id, overlay_cursor, source_instance.backend_factory);
}

auto mf::ExtImageCopyCaptureManagerV1::create_pointer_cursor_session(
    mwrs::Weak<mwrs::ExtImageCaptureSourceV1> const& source,
    [[maybe_unused]] mwrs::Weak<mwrs::Pointer> const& pointer,
    rust::Box<mwrs::ExtImageCopyCaptureCursorSessionV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::ExtImageCopyCaptureCursorSessionV1>
{
    auto& source_instance = ExtImageCaptureSourceV1::from_or_throw(source);
    return std::make_shared<ExtImageCopyCaptureCursorSessionV1>(
        client, std::move(child_instance), child_object_id,
        wayland_executor, cursor_observer_multiplexer, clock, source_instance.cursor_map_position);
}

mf::ExtImageCopyCaptureSessionV1::ExtImageCopyCaptureSessionV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ExtImageCopyCaptureSessionV1Middleware> instance,
    uint32_t object_id,
    bool overlay_cursor,
    ExtImageCopyBackendFactory const& backend_factory)
    : wayland::ExtImageCopyCaptureSessionV1{std::move(client), std::move(instance), object_id},
      backend{backend_factory(this, overlay_cursor)}
{
}

mf::ExtImageCopyCaptureSessionV1::~ExtImageCopyCaptureSessionV1()
{
}

void mf::ExtImageCopyCaptureSessionV1::set_buffer_constraints(geom::Size const &buffer_size)
{
    send_buffer_size_event(
        buffer_size.width.as_uint32_t(), buffer_size.height.as_uint32_t());
    send_shm_format_event(wayland::Shm::Format::argb8888);
    send_done_event();
}

void mf::ExtImageCopyCaptureSessionV1::set_stopped()
{
    if (!stopped)
    {
        send_stopped_event();
        stopped = true;
    }
}

void mf::ExtImageCopyCaptureSessionV1::maybe_capture_frame()
{
    if (!current_frame)
    {
        // Do nothing if there is no frame to capture to.
        return;
    }
    auto& frame = current_frame.value();

    if (!frame.is_ready() || !backend->has_damage())
    {
        // Do nothing if the frame is not ready to capture into or the
        // backend has no reported damage
        return;
    }

    frame.begin_capture(*backend);
}

auto mf::ExtImageCopyCaptureSessionV1::create_frame(
    rust::Box<mwrs::ExtImageCopyCaptureFrameV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::ExtImageCopyCaptureFrameV1>
{
    if (current_frame)
    {
        throw mwrs::ProtocolError{
            object_id(), Error::duplicate_frame,
            "A frame already exists for this session"};
    }
    auto frame = std::make_shared<ExtImageCopyCaptureFrameV1>(
        client, std::move(child_instance), child_object_id, this);
    current_frame = mwrs::make_weak(frame.get());
    return frame;
}

mf::ExtImageCopyCaptureFrameV1::ExtImageCopyCaptureFrameV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ExtImageCopyCaptureFrameV1Middleware> instance,
    uint32_t object_id,
    ExtImageCopyCaptureSessionV1* session)
    : wayland::ExtImageCopyCaptureFrameV1{std::move(client), std::move(instance), object_id},
      session{mwrs::make_weak(session)}
{
}

auto mf::ExtImageCopyCaptureFrameV1::attach_buffer(
    mwrs::Weak<mwrs::Buffer> const& buffer) -> void
{
    if (capture_has_been_called)
    {
        throw mwrs::ProtocolError{
            object_id(), Error::already_captured,
            "Cannot attach buffer after capture"};
    }

    target = buffer;
}

auto mf::ExtImageCopyCaptureFrameV1::damage_buffer(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height) -> void
{
    if (x < 0 || y < 0 || width <= 0 || height <= 0)
    {
        throw mwrs::ProtocolError{
            object_id(), Error::invalid_buffer_damage,
            "invalid buffer damage coordinates"};
    }

    auto new_damage = geom::Rectangle{{x, y}, {width, height}};
    if (frame_damage.size != geom::Size{})
    {
        frame_damage = geom::Rectangles{frame_damage, new_damage}.bounding_rectangle();
    }
    else
    {
        frame_damage = new_damage;
    }
}

auto mf::ExtImageCopyCaptureFrameV1::capture() -> void
{
    if (capture_has_been_called)
    {
        throw mwrs::ProtocolError{
            object_id(), Error::already_captured,
            "capture already called"};
    }
    if (!target)
    {
        throw mwrs::ProtocolError{
            object_id(), Error::no_buffer,
            "no buffer attached"};
    }
    capture_has_been_called = true;

    if (!session)
    {
        send_failed_event(FailureReason::stopped);
        return;
    }
    session.value().maybe_capture_frame();
}

bool mf::ExtImageCopyCaptureFrameV1::is_ready() const
{
    return capture_has_been_called && !capture_has_begun;
}

void mf::ExtImageCopyCaptureFrameV1::begin_capture(ExtImageCopyBackend& backend)
{
    capture_has_begun = true;
    // Check buffer constraints
    auto shm_buffer = dynamic_cast<ShmBuffer*>(mwrs::as_nullable_ptr(target));
    if (!shm_buffer)
    {
        send_failed_event(FailureReason::buffer_constraints);
        return;
    }
    auto shm_data = shm_buffer->data();
    backend.begin_capture(
        shm_data, frame_damage,
        [frame=mwrs::make_weak(this)](ExtImageCopyBackend::CaptureResult const& result)
            {
                if (frame)
                {
                    frame.value().report_result(result);
                }
            });
}

void mf::ExtImageCopyCaptureFrameV1::report_result(
    ExtImageCopyBackend::CaptureResult const& result)
{
    if (!result)
    {
        send_failed_event(result.error());
        return;
    }

    auto const& [captured_time, buffer_space_damage] = result.value();
    send_damage_event(buffer_space_damage.left().as_int(),
                      buffer_space_damage.top().as_int(),
                      buffer_space_damage.size.width.as_int(),
                      buffer_space_damage.size.height.as_int());

    WaylandTimespec const timespec{captured_time};
    send_presentation_time_event(
        timespec.tv_sec_hi,
        timespec.tv_sec_lo,
        timespec.tv_nsec);
    send_ready_event();
}

class mf::ExtImageCopyCaptureCursorSessionV1::CursorObserver
    : public mi::CursorObserver
{
public:
    explicit CursorObserver(ExtImageCopyCaptureCursorSessionV1& session)
        : session{session}
    {
    }
    void cursor_moved_to(float abs_x, float abs_y) override
    {
        session.cursor_moved_to(abs_x, abs_y);
    }
    void pointer_usable() override
    {
        session.pointer_usable();
    }
    void pointer_unusable() override
    {
        session.pointer_unusable();
    }
    void image_set_to(std::shared_ptr<mg::CursorImage>) override
    {
    }

private:
    ExtImageCopyCaptureCursorSessionV1& session;
};

class mf::ExtImageCopyCaptureCursorSessionV1::ImageCopyBackend
    : public ExtImageCopyBackend, public mi::CursorObserver
{
public:
    ImageCopyBackend(ExtImageCopyCaptureSessionV1*session,
                     ExtImageCopyCaptureCursorSessionV1& cursor_session,
                     std::shared_ptr<time::Clock> const& clock);

    void begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        geom::Rectangle const& frame_damage,
        CaptureCallback const& callback) override;

    // From CursorObserver
    void cursor_moved_to(float, float) override {}
    void pointer_usable() override {}
    void pointer_unusable() override {}
    void image_set_to(std::shared_ptr<mg::CursorImage> image) override;

private:
    wayland::Weak<ExtImageCopyCaptureCursorSessionV1> cursor_session;
    std::shared_ptr<time::Clock> const clock;
    std::shared_ptr<mg::CursorImage> cursor_image;
};

mf::ExtImageCopyCaptureCursorSessionV1::ImageCopyBackend::ImageCopyBackend(
    ExtImageCopyCaptureSessionV1* session,
    ExtImageCopyCaptureCursorSessionV1& cursor_session,
    std::shared_ptr<time::Clock> const& clock)
    : ExtImageCopyBackend{session, false},
      cursor_session{&cursor_session},
      clock{clock}
{
}

void mf::ExtImageCopyCaptureCursorSessionV1::ImageCopyBackend::image_set_to(
    std::shared_ptr<mg::CursorImage> image)
{
    cursor_image = image;

    auto const size = image ? image->size() : geom::Size{1, 1};
    if (size != output_space_area.size)
    {
        output_space_area = geom::Rectangle{{0, 0}, size};
        session->set_buffer_constraints(size);
    }
    apply_damage(std::nullopt);
}

void mf::ExtImageCopyCaptureCursorSessionV1::ImageCopyBackend::begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        [[maybe_unused]] geom::Rectangle const& frame_damage,
        CaptureCallback const& callback)
{
    auto const cursor_size = cursor_image ? cursor_image->size() : geom::Size{1, 1};
    geom::Stride const cursor_stride{cursor_size.width.as_int() * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888)};

    using FailureReason = wayland::ExtImageCopyCaptureFrameV1::FailureReason;
    if (shm_data->format() != mir_pixel_format_argb_8888 || shm_data->size() != cursor_size)
    {
        callback(std::unexpected(FailureReason::buffer_constraints));
        return;
    }

    auto mapping = shm_data->map_writeable();
    if (cursor_image)
    {
        auto const dest_stride = mapping->stride();
        auto const cursor_data = static_cast<char const*>(cursor_image->as_argb_8888());
        if (dest_stride == cursor_stride)
        {
            std::memcpy(mapping->data(), cursor_data, mapping->len());
        }
        else
        {
            // strides don't match: copy data in rows
            for (size_t y = 0u; y < cursor_size.height.as_uint32_t(); ++y)
            {
                std::memcpy(mapping->data() + (dest_stride.as_uint32_t() * y),
                       cursor_data + (cursor_stride.as_uint32_t() * y),
                       cursor_stride.as_uint32_t());
            }
        }
    }
    else
    {
        // No cursor set: send a transparent cursor
        std::memset(mapping->data(), 0, mapping->len());
    }

    // The hotspot event on the cursor_session must be sent before the
    // ready event on the image capture session.
    if (cursor_session)
    {
        auto const hotspot = cursor_image ? cursor_image->hotspot() : geom::Displacement{0, 0};
        cursor_session.value().send_hotspot_event(hotspot.dx.as_int(), hotspot.dy.as_int());
    }

    callback({{clock->now(), {{0, 0}, cursor_size}}});
    damage_amount = DamageAmount::none;
}

mf::ExtImageCopyCaptureCursorSessionV1::ExtImageCopyCaptureCursorSessionV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ExtImageCopyCaptureCursorSessionV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock,
    ExtImageCopyCursorMapPosition const& map_position)
    : wayland::ExtImageCopyCaptureCursorSessionV1{std::move(client), std::move(instance), object_id},
      wayland_executor{wayland_executor},
      cursor_observer_multiplexer{cursor_observer_multiplexer},
      cursor_observer{std::make_shared<CursorObserver>(*this)},
      clock{clock},
      map_position{map_position}
{
    cursor_observer_multiplexer->register_interest(cursor_observer, *wayland_executor);
}

mf::ExtImageCopyCaptureCursorSessionV1::~ExtImageCopyCaptureCursorSessionV1()
{
    cursor_observer_multiplexer->unregister_interest(*cursor_observer);
}

auto mf::ExtImageCopyCaptureCursorSessionV1::get_capture_session(
    rust::Box<mwrs::ExtImageCopyCaptureSessionV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::ExtImageCopyCaptureSessionV1>
{
    if (cursor_image_session)
    {
        throw mwrs::ProtocolError{
            object_id(), Error::duplicate_session,
            "An ext_image_copy_capture_session_v1 already exists for this "
            "ext_image_copy_capture_cursor_session_v1"};
    }
    auto backend_factory = [this](auto *session, [[maybe_unused]] bool overlay_cursor)
        {
            auto backend = std::make_shared<ImageCopyBackend>(session, *this, clock);
            cursor_observer_multiplexer->register_interest(backend, *wayland_executor);
            return backend;
        };
    auto session = std::make_shared<ExtImageCopyCaptureSessionV1>(
        client, std::move(child_instance), child_object_id, false, backend_factory);
    cursor_image_session = mwrs::make_weak(session.get());
    return session;
}

void mf::ExtImageCopyCaptureCursorSessionV1::cursor_moved_to(
    float abs_x, float abs_y)
{
    bool was_visible = pointer_is_usable && pointer_in_source;
    auto position = map_position(abs_x, abs_y);
    pointer_in_source = bool(position);
    if (position) {
        if (!was_visible && pointer_is_usable)
        {
            send_enter_event();
        }
        auto const& point = position.value();
        send_position_event(point.x.as_int(), point.y.as_int());
    }
    else if (was_visible)
    {
        send_leave_event();
    }
}

void mf::ExtImageCopyCaptureCursorSessionV1::pointer_usable()
{
    bool was_visible = pointer_is_usable && pointer_in_source;
    pointer_is_usable = true;

    if (!was_visible && pointer_in_source)
    {
        send_enter_event();
    }
}

void mf::ExtImageCopyCaptureCursorSessionV1::pointer_unusable()
{
    bool was_visible = pointer_is_usable && pointer_in_source;
    pointer_is_usable = false;

    if (was_visible)
    {
        send_leave_event();
    }
}
