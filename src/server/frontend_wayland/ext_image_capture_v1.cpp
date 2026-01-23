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
#include <mir/compositor/screen_shooter.h>
#include <mir/compositor/screen_shooter_factory.h>
#include <mir/fatal.h>
#include <mir/frontend/surface_stack.h>
#include <mir/geometry/rectangles.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/scene/scene_change_notification.h>
#include <mir/time/clock.h>
#include <mir/wayland/protocol_error.h>
#include <mir/wayland/weak.h>
#include "output_manager.h"
#include "shm.h"
#include "wayland_timespec.h"

#include <expected>
#include <tuple>

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{
auto translate_and_scale(
    geom::Rectangle rect,
    geom::Rectangle input_space,
    geom::Rectangle output_space) -> geom::Rectangle
{
    auto const x_scale = static_cast<double>(output_space.size.width.as_value()) / input_space.size.width.as_value();
    auto const y_scale = static_cast<double>(output_space.size.height.as_value()) / input_space.size.height.as_value();
    rect.size.width = rect.size.width * x_scale;
    rect.size.height = rect.size.height * y_scale;
    auto displacement = rect.top_left - input_space.top_left;
    rect.top_left.x = output_space.top_left.x + displacement.dx * x_scale;
    rect.top_left.y = output_space.top_left.y + displacement.dy * y_scale;
    return rect;
}
}

namespace mir::frontend
{

struct ExtImageCaptureV1Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<compositor::ScreenShooterFactory> const screen_shooter_factory;
    std::shared_ptr<SurfaceStack> const surface_stack;
};

/* Image copy backends */
class ExtImageCopyCaptureSessionV1;

class ExtImageCopyBackend
{
public:
    using CaptureResult = std::expected<std::tuple<time::Timestamp, geom::Rectangle>, uint32_t>;
    using CaptureCallback = std::function<void(CaptureResult const&)>;

    ExtImageCopyBackend(ExtImageCopyCaptureSessionV1* session, bool overlay_cursor);
    virtual ~ExtImageCopyBackend() = default;

    ExtImageCopyBackend(ExtImageCopyBackend const&) = delete;
    ExtImageCopyBackend& operator=(ExtImageCopyBackend const&) = delete;

    virtual bool has_damage();

    // \pre has_damage() == true
    virtual void begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        geom::Rectangle const& frame_damage,
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

    geom::Rectangle output_space_area;
    geom::Rectangle output_space_damage;
    DamageAmount damage_amount = DamageAmount::full;

    void apply_damage(std::optional<geom::Rectangle> const& damage);
};

class ExtImageCopyCaptureCursorSessionV1;

class ExtOutputImageCopyBackend
    : public ExtImageCopyBackend, public OutputConfigListener
{
public:
    ExtOutputImageCopyBackend(ExtImageCopyCaptureSessionV1 *session, bool overlay_cursor, OutputGlobal* output, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);
    ~ExtOutputImageCopyBackend();

    bool has_damage() override;
    void begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        geom::Rectangle const& frame_damage,
        CaptureCallback const& callback) override;

private:
    wayland::Weak<OutputGlobal> const output;
    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;

    wayland::DestroyListenerId destroy_listener_id;
    std::shared_ptr<scene::SceneChangeNotification> change_notifier;
    std::unique_ptr<compositor::ScreenShooter> const screen_shooter;

    void create_change_notifier();
    auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool override;
};

using ExtImageCopyBackendFactory = std::function<std::unique_ptr<ExtImageCopyBackend>(ExtImageCopyCaptureSessionV1*,bool)>;
using ExtImageCopyCursorMapPosition = std::function<std::optional<geom::Point>(float abs_x, float abs_y)>;

/* Image capture sources */
class ExtOutputImageCaptureSourceManagerV1Global
    : public wayland::OutputImageCaptureSourceManagerV1::Global
{
public:
    ExtOutputImageCaptureSourceManagerV1Global(wl_display* display, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void bind(wl_resource *new_resource) override;

    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

class ExtOutputImageCaptureSourceManagerV1
    : public wayland::OutputImageCaptureSourceManagerV1
{
public:
    ExtOutputImageCaptureSourceManagerV1(wl_resource *resource, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void create_source(wl_resource* new_resource, wl_resource* output) override;

    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

class ExtImageCaptureSourceV1
    : public wayland::ImageCaptureSourceV1
{
public:
    ExtImageCaptureSourceV1(wl_resource* resource,
                            ExtImageCopyBackendFactory const& backend_factory,
                            ExtImageCopyCursorMapPosition const& cursor_map_position);

    static ExtImageCaptureSourceV1* from_or_throw(wl_resource* resource);

    ExtImageCopyBackendFactory const backend_factory;
    ExtImageCopyCursorMapPosition const cursor_map_position;
};


/* Image capture sessions */
class ExtImageCopyCaptureManagerV1Global
    : public wayland::ImageCopyCaptureManagerV1::Global
{
public:
    ExtImageCopyCaptureManagerV1Global(
        wl_display* display,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
        std::shared_ptr<time::Clock> const& clock);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<input::CursorObserverMultiplexer> const cursor_observer_multiplexer;
    std::shared_ptr<time::Clock> const clock;
};

class ExtImageCopyCaptureManagerV1
    : public wayland::ImageCopyCaptureManagerV1
{
public:
    ExtImageCopyCaptureManagerV1(wl_resource* resource,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
        std::shared_ptr<time::Clock> const& clock);

private:
    void create_session(wl_resource* new_resource, wl_resource* source, uint32_t options) override;
    void create_pointer_cursor_session(wl_resource* new_resource, wl_resource* source, wl_resource* pointer) override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<input::CursorObserverMultiplexer> const cursor_observer_multiplexer;
    std::shared_ptr<time::Clock> const clock;
};

class ExtImageCopyCaptureFrameV1;

class ExtImageCopyCaptureSessionV1
    : public wayland::ImageCopyCaptureSessionV1
{
public:
    ExtImageCopyCaptureSessionV1(wl_resource* resource, bool overlay_cursor, ExtImageCopyBackendFactory const& backend_factory);
    ~ExtImageCopyCaptureSessionV1();

    void set_buffer_constraints(geom::Size const& buffer_size);
    void set_stopped();
    void maybe_capture_frame();

private:
    void create_frame(wl_resource* new_resource) override;

    bool stopped = false;
    wayland::Weak<ExtImageCopyCaptureFrameV1> current_frame;

    std::unique_ptr<ExtImageCopyBackend> backend;
};

class ExtImageCopyCaptureFrameV1
    : public wayland::ImageCopyCaptureFrameV1
{
public:
    ExtImageCopyCaptureFrameV1(wl_resource* resource, ExtImageCopyCaptureSessionV1* session);

    bool is_ready() const;
    // \pre backend.has_damage() == true
    void begin_capture(ExtImageCopyBackend& backend);
    void report_result(ExtImageCopyBackend::CaptureResult const& result);

private:
    void attach_buffer(wl_resource* buffer) override;
    void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void capture() override;

    bool capture_has_been_called = false;
    bool capture_has_begun = false;
    wayland::Weak<wayland::Buffer> target;
    geom::Rectangle frame_damage;

    wayland::Weak<ExtImageCopyCaptureSessionV1> const session;
};

class ExtImageCopyCaptureCursorSessionV1
    : public wayland::ImageCopyCaptureCursorSessionV1
{
public:
  ExtImageCopyCaptureCursorSessionV1(
      wl_resource* resource, Executor& wayland_executor,
      std::shared_ptr<input::CursorObserverMultiplexer> const&
          cursor_observer_multiplexer,
      std::shared_ptr<time::Clock> const& clock,
      ExtImageCopyCursorMapPosition const& map_position);
  ~ExtImageCopyCaptureCursorSessionV1();

private:
    class CursorObserver;
    class ImageCopyBackend;

    void get_capture_session(wl_resource* session) override;

    // From CursorObserver
    void cursor_moved_to(float abs_x, float abs_y);
    void pointer_usable();
    void pointer_unusable();

    bool pointer_is_usable = true;
    bool pointer_in_source = false;
    wayland::Weak<ExtImageCopyCaptureSessionV1> cursor_image_session;

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

mf::ExtOutputImageCopyBackend::ExtOutputImageCopyBackend(
    ExtImageCopyCaptureSessionV1* session,
    bool overlay_cursor,
    OutputGlobal* output,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx)
    : ExtImageCopyBackend{session, overlay_cursor},
      output{output},
      ctx{ctx},
      screen_shooter(ctx->screen_shooter_factory->create(thread_pool_executor))
{
    if (output)
    {
        output->add_listener(this);
        destroy_listener_id = output->add_destroy_listener([session]()
            {
                session->set_stopped();
            });
        output_config_changed(output->current_config());
    }
    else
    {
        session->set_stopped();
    }
}

mf::ExtOutputImageCopyBackend::~ExtOutputImageCopyBackend()
{
    if (output)
    {
        output.value().remove_listener(this);
        output.value().remove_destroy_listener(destroy_listener_id);
    }
    if (change_notifier)
    {
        ctx->surface_stack->remove_observer(change_notifier);
    }
}

auto mf::ExtOutputImageCopyBackend::output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool
{
    // Send buffer constraints matching new output configuration
    auto const buffer_size = config.modes[config.current_mode_index].size;
    session->set_buffer_constraints(buffer_size);

    // Mark the whole buffer as damaged
    output_space_area = output.value().current_config().extents();
    apply_damage(std::nullopt);
    return true;
}

bool mf::ExtOutputImageCopyBackend::has_damage()
{
    if (!change_notifier)
    {
        // Create the change notifier the first time a client tries to
        // capture a frame.
        create_change_notifier();
    }

    return ExtImageCopyBackend::has_damage();
}

void mf::ExtOutputImageCopyBackend::create_change_notifier()
{
    auto callback = [this, weak_self=wayland::make_weak(this)](std::optional<geom::Rectangle> const& damage)
        {
            ctx->wayland_executor->spawn([weak_self, damage]()
                {
                    if (weak_self)
                    {
                        weak_self.value().apply_damage(damage);
                    }
                });
        };
    change_notifier = std::make_shared<ms::SceneChangeNotification>(
        [callback](){ callback(std::nullopt); },
        callback);
    ctx->surface_stack->add_observer(change_notifier);
}

void mf::ExtOutputImageCopyBackend::begin_capture(
    std::shared_ptr<renderer::software::RWMappable> const& shm_data,
    [[maybe_unused]] geom::Rectangle const& frame_damage,
    CaptureCallback const& callback)
{
    using FailureReason = wayland::ImageCopyCaptureFrameV1::FailureReason;
    if (!output)
    {
        callback(std::unexpected(FailureReason::stopped));
        return;
    }

    auto const& output_config = output.value().current_config();
    auto const buffer_size = output_config.modes[output_config.current_mode_index].size;
    auto const output_space_area = output_config.extents();
    auto const transform = output_config.transformation();

    // Check that the provided buffer matches our constraints
    if (!shm_data || shm_data->format() != mir_pixel_format_argb_8888 || shm_data->size() != buffer_size)
    {
        callback(std::unexpected(FailureReason::buffer_constraints));
        return;
    }

    geom::Rectangle buffer_space_damage;
    switch (damage_amount)
    {
    case DamageAmount::none:
        // This should never happen as maybe_capture_frame() checks has_damage() before calling begin_capture()
        fatal_error_abort("begin_capture() called with no damage - this is a precondition violation");
        break;

    case DamageAmount::partial:
        buffer_space_damage = translate_and_scale(
            output_space_damage,
            output_space_area,
            {{}, buffer_size});
        break;

    case DamageAmount::full:
        buffer_space_damage = {{}, buffer_size};
        break;
    }

    // TODO: capture only the region covered by
    // union(buffer_space_damage, frame->frame_damage)

    screen_shooter->capture(
        shm_data, output_space_area, transform, overlay_cursor,
        [executor=ctx->wayland_executor, buffer_space_damage, callback](std::optional<time::Timestamp> captured_time)
        {
            executor->spawn([callback, captured_time, buffer_space_damage]()
                {
                    if (captured_time)
                    {
                        callback({{captured_time.value(), buffer_space_damage}});
                    }
                    else
                    {
                        callback(std::unexpected(FailureReason::unknown));
                    }
                });
        });

    damage_amount = DamageAmount::none;
}

/* Image capture sources */

auto mf::create_ext_output_image_capture_source_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<compositor::ScreenShooterFactory> const& screen_shooter_factory,
    std::shared_ptr<SurfaceStack> const& surface_stack)
-> std::shared_ptr<wayland::OutputImageCaptureSourceManagerV1::Global>
{
    auto ctx = std::make_shared<ExtImageCaptureV1Ctx>(
        wayland_executor, screen_shooter_factory, surface_stack);
    return std::make_shared<ExtOutputImageCaptureSourceManagerV1Global>(display, ctx);
}

mf::ExtOutputImageCaptureSourceManagerV1Global::ExtOutputImageCaptureSourceManagerV1Global(
    wl_display* display,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx)
    : Global{display, Version<1>()},
      ctx{ctx}
{
}

void mf::ExtOutputImageCaptureSourceManagerV1Global::bind(wl_resource* new_resource)
{
    new ExtOutputImageCaptureSourceManagerV1{new_resource, ctx};
}

mf::ExtOutputImageCaptureSourceManagerV1::ExtOutputImageCaptureSourceManagerV1(
    wl_resource* resource,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx)
    : wayland::OutputImageCaptureSourceManagerV1{resource, Version<1>()},
      ctx{ctx}
{
}

void mf::ExtOutputImageCaptureSourceManagerV1::create_source(wl_resource* new_resource, wl_resource* output)
{
    auto& output_global = OutputGlobal::from_or_throw(output);
    ExtImageCopyBackendFactory backend_factory = [output=wayland::make_weak(&output_global), ctx=ctx](auto *session, bool overlay_cursor) {
        return std::make_unique<ExtOutputImageCopyBackend>(session, overlay_cursor, wayland::as_nullable_ptr(output), ctx);
    };
    ExtImageCopyCursorMapPosition cursor_map_position = [output=wayland::make_weak(&output_global)](float abs_x, float abs_y) -> std::optional<geom::Point> {
        if (!output)
        {
            return std::nullopt;
        }
        auto const config = output.value().current_config();
        auto const output_space_area = config.extents();
        geom::Point const abs_pos{abs_x, abs_y};
        if (!output_space_area.contains(abs_pos))
        {
            return std::nullopt;
        }

        // Translate pointer coordinates to buffer space. We don't take
        // rotations/flips into account because our image copy backend is
        // producing untransformed frames.
        auto const buffer_size = config.modes[config.current_mode_index].size;
        auto const x_scale = static_cast<double>(buffer_size.width.as_value()) / output_space_area.size.width.as_value();
        auto const y_scale = static_cast<double>(buffer_size.height.as_value()) / output_space_area.size.height.as_value();

        auto const raw_offset = abs_pos - output_space_area.top_left;
        decltype(raw_offset) const scaled_offset{x_scale * raw_offset.dx, y_scale * raw_offset.dy};
        return as_point(scaled_offset);
    };
    new ExtImageCaptureSourceV1{new_resource, backend_factory, cursor_map_position};
}


mf::ExtImageCaptureSourceV1::ExtImageCaptureSourceV1(
    wl_resource* resource,
    ExtImageCopyBackendFactory const& backend_factory,
    ExtImageCopyCursorMapPosition const& cursor_map_position)
    : wayland::ImageCaptureSourceV1{resource, Version<1>()},
          backend_factory{backend_factory},
          cursor_map_position{cursor_map_position}
{
}

mf::ExtImageCaptureSourceV1* mf::ExtImageCaptureSourceV1::from_or_throw(wl_resource *resource)
{
    auto source = dynamic_cast<ExtImageCaptureSourceV1*>(wayland::ImageCaptureSourceV1::from(resource));

    if (!source)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "ext_image_capture_source_v1@" +
            std::to_string(wl_resource_get_id(resource)) +
            " is not a mir::frontend::ExtImageCaptureSourceV1"));
    }
    return source;
}

/* Image capture sessions */

auto mf::create_ext_image_copy_capture_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock)
-> std::shared_ptr<wayland::ImageCopyCaptureManagerV1::Global>
{
    return std::make_shared<ExtImageCopyCaptureManagerV1Global>(
        display, wayland_executor, cursor_observer_multiplexer, clock);
}

mf::ExtImageCopyCaptureManagerV1Global::ExtImageCopyCaptureManagerV1Global(
    wl_display *display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock)
    : Global{display, Version<1>()},
      wayland_executor{wayland_executor},
      cursor_observer_multiplexer{cursor_observer_multiplexer},
      clock{clock}
{
}

void mf::ExtImageCopyCaptureManagerV1Global::bind(wl_resource *new_resource)
{
    new ExtImageCopyCaptureManagerV1{new_resource, wayland_executor, cursor_observer_multiplexer, clock};
}

mf::ExtImageCopyCaptureManagerV1::ExtImageCopyCaptureManagerV1(
    wl_resource *resource,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock)
    : wayland::ImageCopyCaptureManagerV1{resource, Version<1>()},
      wayland_executor{wayland_executor},
      cursor_observer_multiplexer{cursor_observer_multiplexer},
      clock{clock}
{
}

void mf::ExtImageCopyCaptureManagerV1::create_session(
    wl_resource* new_resource, wl_resource* source, uint32_t options)
{
    auto const source_instance = ExtImageCaptureSourceV1::from_or_throw(source);
    bool overlay_cursor = (options & Options::paint_cursors) != 0;

    new ExtImageCopyCaptureSessionV1{new_resource, overlay_cursor, source_instance->backend_factory};
}

void mf::ExtImageCopyCaptureManagerV1::create_pointer_cursor_session(
    wl_resource* new_resource,
    wl_resource* source,
    [[maybe_unused]] wl_resource* pointer)
{
    auto const source_instance = ExtImageCaptureSourceV1::from_or_throw(source);
    new ExtImageCopyCaptureCursorSessionV1{new_resource, *wayland_executor, cursor_observer_multiplexer, clock, source_instance->cursor_map_position};
}

mf::ExtImageCopyCaptureSessionV1::ExtImageCopyCaptureSessionV1(
    wl_resource *resource,
    bool overlay_cursor,
    ExtImageCopyBackendFactory const& backend_factory)
    : wayland::ImageCopyCaptureSessionV1{resource, Version<1>()},
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

void mf::ExtImageCopyCaptureSessionV1::create_frame(
    wl_resource* new_resource)
{
    if (current_frame)
    {
        BOOST_THROW_EXCEPTION(wayland::ProtocolError(
            resource, Error::duplicate_frame,
            "A frame already exists for this session"));
    }
    current_frame = wayland::make_weak(
        new ExtImageCopyCaptureFrameV1{new_resource, this});
}

mf::ExtImageCopyCaptureFrameV1::ExtImageCopyCaptureFrameV1(
    wl_resource *resource,
    ExtImageCopyCaptureSessionV1* session)
    : wayland::ImageCopyCaptureFrameV1{resource, Version<1>()},
      session{session}
{
}

void mf::ExtImageCopyCaptureFrameV1::attach_buffer(
    wl_resource* buffer)
{
    if (capture_has_been_called)
    {
        BOOST_THROW_EXCEPTION(wayland::ProtocolError(
            resource, Error::already_captured,
            "Cannot attach buffer after capture"));
    }

    auto buf = wayland::Buffer::from(buffer);
    if (!buf)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "wl_buffer@" +
            std::to_string(wl_resource_get_id(buffer)) +
            " is not a wayland::Buffer"));
    }
    target = wayland::make_weak(buf);
}

void mf::ExtImageCopyCaptureFrameV1::damage_buffer(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height)
{
    if (x < 0 || y < 0 || width <= 0 || height <= 0)
    {
        BOOST_THROW_EXCEPTION(wayland::ProtocolError(
            resource, Error::invalid_buffer_damage,
            "invalid buffer damage coordinates"));
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

void mf::ExtImageCopyCaptureFrameV1::capture()
{
    if (capture_has_been_called)
    {
        BOOST_THROW_EXCEPTION(wayland::ProtocolError(
            resource, Error::already_captured,
            "capture already called"));
    }
    if (!target)
    {
        BOOST_THROW_EXCEPTION(wayland::ProtocolError(
            resource, Error::no_buffer,
            "no buffer attached"));
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
    auto shm_buffer = dynamic_cast<ShmBuffer*>(wayland::as_nullable_ptr(target));
    if (!shm_buffer)
    {
        send_failed_event(FailureReason::buffer_constraints);
        return;
    }
    auto shm_data = shm_buffer->data();
    backend.begin_capture(
        shm_data, frame_damage,
        [frame=wayland::make_weak(this)](ExtImageCopyBackend::CaptureResult const& result)
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
    CursorObserver(ExtImageCopyCaptureCursorSessionV1* session)
        : session{session}
    {
    }
    void cursor_moved_to(float abs_x, float abs_y) override
    {
        if (session)
        {
            session.value().cursor_moved_to(abs_x, abs_y);
        }
    }
    void pointer_usable() override
    {
        if (session)
        {
            session.value().pointer_usable();
        }
    }
    void pointer_unusable() override
    {
        if (session)
        {
            session.value().pointer_unusable();
        }
    }
    void image_set_to(std::shared_ptr<mg::CursorImage>) override
    {
    }

private:
    wayland::Weak<ExtImageCopyCaptureCursorSessionV1> const session;
};

class mf::ExtImageCopyCaptureCursorSessionV1::ImageCopyBackend
    : public ExtImageCopyBackend
{
public:
    ImageCopyBackend(ExtImageCopyCaptureSessionV1 *session,
                     std::shared_ptr<time::Clock> const& clock);

    void begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        geom::Rectangle const& frame_damage,
        CaptureCallback const& callback) override;

private:
    std::shared_ptr<time::Clock> const clock;
};

mf::ExtImageCopyCaptureCursorSessionV1::ImageCopyBackend::ImageCopyBackend(
    ExtImageCopyCaptureSessionV1 *session,
    std::shared_ptr<time::Clock> const& clock)
    : ExtImageCopyBackend{session, false},
      clock{clock}
{
    output_space_area = geom::Rectangle{{0, 0}, {64, 64}};
    session->set_buffer_constraints(output_space_area.size);
}

void mf::ExtImageCopyCaptureCursorSessionV1::ImageCopyBackend::begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        [[maybe_unused]] geom::Rectangle const& frame_damage,
        CaptureCallback const& callback)
{
    auto mapping = shm_data->map_writeable();
    auto data = mapping->data();
    auto size = mapping->size();
    auto stride = mapping->stride();
    for (int y = 0; y < size.height.as_int(); y++)
    {
        for (int x = 0; x < size.width.as_int(); x++)
        {
            auto pos = y * stride.as_int() + x * 4;

            if (x <= y)
            {
                data[pos] = std::byte{0xff};
                data[pos+1] = std::byte{0xff};
                data[pos+2] = std::byte{0xff};
                data[pos+3] = std::byte{0xff};
            }
            else
            {
                data[pos] = std::byte{0x00};
                data[pos+1] = std::byte{0x00};
                data[pos+2] = std::byte{0x00};
                data[pos+3] = std::byte{0x00};
            }
        }
    }
    callback({{clock->now(), {{0, 0}, size}}});
    damage_amount = DamageAmount::none;
}

mf::ExtImageCopyCaptureCursorSessionV1::ExtImageCopyCaptureCursorSessionV1(
    wl_resource* resource,
    Executor& wayland_executor,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
    std::shared_ptr<time::Clock> const& clock,
    ExtImageCopyCursorMapPosition const& map_position)
    : wayland::ImageCopyCaptureCursorSessionV1{resource, Version<1>{}},
      cursor_observer_multiplexer{cursor_observer_multiplexer},
      cursor_observer{std::make_shared<CursorObserver>(this)},
      clock{clock},
      map_position{map_position}
{
    cursor_observer_multiplexer->register_interest(cursor_observer, wayland_executor);
    // TODO: placeholder until we have the hotspot of the real cursor
    send_hotspot_event(0, 0);
}

mf::ExtImageCopyCaptureCursorSessionV1::~ExtImageCopyCaptureCursorSessionV1()
{
    cursor_observer_multiplexer->unregister_interest(*cursor_observer);
}

void mf::ExtImageCopyCaptureCursorSessionV1::get_capture_session(
    [[maybe_unused]] wl_resource* session)
{
    if (cursor_image_session)
    {
        BOOST_THROW_EXCEPTION(wayland::ProtocolError(
            resource, Error::duplicate_session,
            "An ext_image_copy_capture_session_v1 already exists for this "
            "ext_image_copy_capture_cursor_session_v1"));
    }
    auto backend_factory = [clock=clock](auto *session, [[maybe_unused]] bool overlay_cursor)
        {
            return std::make_unique<ImageCopyBackend>(session, clock);
        };
    cursor_image_session = wayland::make_weak(
        new ExtImageCopyCaptureSessionV1{session, false, backend_factory});
}

// From CursorObserver
void mf::ExtImageCopyCaptureCursorSessionV1::cursor_moved_to(
    float abs_x, float abs_y)
{
    // TODO: offset position based on position of source (output, toplevel, etc)
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
