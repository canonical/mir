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

#include "mir/executor.h"
#include "mir/compositor/screen_shooter.h"
#include "mir/compositor/screen_shooter_factory.h"
#include "mir/frontend/surface_stack.h"
#include "mir/geometry/rectangles.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/scene/scene_change_notification.h"
#include "mir/wayland/protocol_error.h"
#include "mir/wayland/weak.h"
#include "output_manager.h"
#include "shm.h"
#include "wayland_timespec.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
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

namespace mir::frontend {

/* Image capture sources */

class ExtOutputImageCaptureSourceManagerV1Global
    : public wayland::OutputImageCaptureSourceManagerV1::Global
{
public:
    ExtOutputImageCaptureSourceManagerV1Global(wl_display* display);

private:
    void bind(wl_resource *new_resource) override;
};

class ExtOutputImageCaptureSourceManagerV1
    : public wayland::OutputImageCaptureSourceManagerV1
{
public:
    ExtOutputImageCaptureSourceManagerV1(wl_resource *resource);

private:
    void create_source(wl_resource* new_resource, wl_resource* output) override;
};

class ExtImageCaptureSourceV1
    : public wayland::ImageCaptureSourceV1
{
public:
    ExtImageCaptureSourceV1(wl_resource* resource,
                            OutputGlobal& output,
                            ExtOutputImageCaptureSourceManagerV1* manager);

    static ExtImageCaptureSourceV1* from_or_throw(wl_resource* resource);

    // FIXME: currently this is only set up for output capture. It
    // will need to change to also cover toplevel capture.
    wayland::Weak<OutputGlobal> const output;

private:
    wayland::Weak<ExtOutputImageCaptureSourceManagerV1> const manager;
};


/* Image capture sessions */
struct ExtImageCaptureV1Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<compositor::ScreenShooterFactory> const screen_shooter_factory;
    std::shared_ptr<SurfaceStack> const surface_stack;
};

class ExtImageCopyCaptureManagerV1Global
    : public wayland::ImageCopyCaptureManagerV1::Global
{
public:
    ExtImageCopyCaptureManagerV1Global(wl_display* display, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

class ExtImageCopyCaptureManagerV1
    : public wayland::ImageCopyCaptureManagerV1
{
public:
    ExtImageCopyCaptureManagerV1(wl_resource* resource, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void create_session(wl_resource* new_resource, wl_resource* source, uint32_t options) override;
    void create_pointer_cursor_session(wl_resource* new_resource, wl_resource* source, wl_resource* pointer) override;

    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

class ExtImageCopyCaptureFrameV1;

class ExtImageCopyCaptureSessionV1
    : public wayland::ImageCopyCaptureSessionV1, OutputConfigListener
{
public:
    ExtImageCopyCaptureSessionV1(wl_resource* resource, ExtImageCaptureSourceV1* source, uint32_t options, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);
    ~ExtImageCopyCaptureSessionV1();

    void maybe_capture_frame();

    std::unique_ptr<compositor::ScreenShooter> const screen_shooter;
private:
    enum class DamageAmount
    {
        none,
        partial,
        full,
    };

    void create_frame(wl_resource* new_resource) override;
    auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool override;

    void create_change_notifier();
    void apply_damage(std::optional<geom::Rectangle> const& damage);

    wayland::Weak<OutputGlobal> const output;
    bool overlay_cursor;
    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;

    DamageAmount damage_amount = DamageAmount::full;
    geom::Rectangle output_space_damage;
    wayland::Weak<ExtImageCopyCaptureFrameV1> current_frame;
    std::shared_ptr<scene::SceneChangeNotification> change_notifier;
};

class ExtImageCopyCaptureFrameV1
    : public wayland::ImageCopyCaptureFrameV1
{
public:
    ExtImageCopyCaptureFrameV1(wl_resource* resource, ExtImageCopyCaptureSessionV1* session, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

    bool is_ready() const;

    void capture_frame(geom::Rectangle output_space_area, glm::mat2 transform, bool overlay_cursor, geom::Size buffer_size, geom::Rectangle buffer_space_damage);

private:
    void attach_buffer(wl_resource* buffer) override;
    void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void capture() override;

    void report_result(std::optional<time::Timestamp> captured_time,
                       geom::Rectangle buffer_space_damage);

    bool capture_has_been_called = false;
    wayland::Weak<wayland::Buffer> target;
    geom::Rectangle frame_damage;

    wayland::Weak<ExtImageCopyCaptureSessionV1> const session;
    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

}

auto mf::create_ext_output_image_capture_source_manager_v1(
    wl_display* display)
-> std::shared_ptr<wayland::OutputImageCaptureSourceManagerV1::Global>
{
    return std::make_shared<ExtOutputImageCaptureSourceManagerV1Global>(display);;
}

mf::ExtOutputImageCaptureSourceManagerV1Global::ExtOutputImageCaptureSourceManagerV1Global(
    wl_display* display)
    : Global{display, Version<1>()}
{
}

void mf::ExtOutputImageCaptureSourceManagerV1Global::bind(wl_resource* new_resource)
{
    new ExtOutputImageCaptureSourceManagerV1{new_resource};
}

mf::ExtOutputImageCaptureSourceManagerV1::ExtOutputImageCaptureSourceManagerV1(
    wl_resource* resource)
    : wayland::OutputImageCaptureSourceManagerV1{resource, Version<1>()}
{
}

void mf::ExtOutputImageCaptureSourceManagerV1::create_source(wl_resource* new_resource, wl_resource* output)
{
    auto& output_global = OutputGlobal::from_or_throw(output);
    new ExtImageCaptureSourceV1{new_resource, output_global, this};
}

mf::ExtImageCaptureSourceV1::ExtImageCaptureSourceV1(
    wl_resource* resource, OutputGlobal& output,
    ExtOutputImageCaptureSourceManagerV1 *manager)
    : wayland::ImageCaptureSourceV1{resource, Version<1>()},
      output{&output},
      manager{manager}
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

auto mf::create_ext_image_copy_capture_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<compositor::ScreenShooterFactory> const& screen_shooter_factory,
    std::shared_ptr<SurfaceStack> const& surface_stack)
-> std::shared_ptr<wayland::ImageCopyCaptureManagerV1::Global>
{
    auto ctx = std::make_shared<ExtImageCaptureV1Ctx>(
        wayland_executor, allocator, screen_shooter_factory, surface_stack);
    return std::make_shared<ExtImageCopyCaptureManagerV1Global>(display, std::move(ctx));;
}

mf::ExtImageCopyCaptureManagerV1Global::ExtImageCopyCaptureManagerV1Global(
    wl_display *display,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx)
    : Global{display, Version<1>()},
      ctx{ctx}
{
}

void mf::ExtImageCopyCaptureManagerV1Global::bind(wl_resource *new_resource)
{
    new ExtImageCopyCaptureManagerV1{new_resource, ctx};
}

mf::ExtImageCopyCaptureManagerV1::ExtImageCopyCaptureManagerV1(
    wl_resource *resource,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx)
    : wayland::ImageCopyCaptureManagerV1{resource, Version<1>()},
      ctx{ctx}
{
}

void mf::ExtImageCopyCaptureManagerV1::create_session(
    wl_resource* new_resource, wl_resource* source, uint32_t options)
{
    auto const source_instance = ExtImageCaptureSourceV1::from_or_throw(source);
    new ExtImageCopyCaptureSessionV1{new_resource, source_instance, options, ctx};
}

void mf::ExtImageCopyCaptureManagerV1::create_pointer_cursor_session(
    [[maybe_unused]] wl_resource* new_resource,
    [[maybe_unused]] wl_resource* source,
    [[maybe_unused]] wl_resource* pointer)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("not implemented"));
}

mf::ExtImageCopyCaptureSessionV1::ExtImageCopyCaptureSessionV1(
    wl_resource *resource,
    ExtImageCaptureSourceV1* source,
    uint32_t options,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx)
    : wayland::ImageCopyCaptureSessionV1{resource, Version<1>()},
      screen_shooter(ctx->screen_shooter_factory->create(thread_pool_executor)),
      output{source->output},
      overlay_cursor{(options & wayland::ImageCopyCaptureManagerV1::Options::paint_cursors) != 0},
      ctx{ctx}
{
    if (output)
    {
        output.value().add_listener(this);
        output_config_changed(output.value().current_config());
    }
}

mf::ExtImageCopyCaptureSessionV1::~ExtImageCopyCaptureSessionV1()
{
    if (output)
    {
        output.value().remove_listener(this);
    }
    if (change_notifier)
    {
        ctx->surface_stack->remove_observer(change_notifier);
    }
}

auto mf::ExtImageCopyCaptureSessionV1::output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool
{
    // Send buffer constraints matching new output configuration
    auto const buffer_size = config.modes[config.current_mode_index].size;
    send_buffer_size_event(
        buffer_size.width.as_uint32_t(), buffer_size.height.as_uint32_t());
    send_shm_format_event(wayland::Shm::Format::argb8888);
    send_done_event();
    return true;
}

void mf::ExtImageCopyCaptureSessionV1::apply_damage(
    std::optional<geom::Rectangle> const& damage)
{
    if (!output) {
        return;
    }

    if (damage && damage_amount != DamageAmount::full)
    {
        auto const output_space_area = output.value().current_config().extents();

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
        maybe_capture_frame();
    }
}

void mf::ExtImageCopyCaptureSessionV1::create_change_notifier()
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

void mf::ExtImageCopyCaptureSessionV1::maybe_capture_frame()
{
    if (!current_frame || !current_frame.value().is_ready())
    {
        // Do nothing if there is no frame to capture to.
        return;
    }

    if (!change_notifier)
    {
        // Create the change notifier the first time a client tries to
        // capture a frame.
        create_change_notifier();
    }

    auto const& output_config = output.value().current_config();
    auto const buffer_size = output_config.modes[output_config.current_mode_index].size;
    auto const output_space_area = output_config.extents();
    auto const transform = output_config.transformation();

    switch (damage_amount)
    {
    case DamageAmount::none:
        return;

    case DamageAmount::partial:
        current_frame.value().capture_frame(
            output_space_area,
            transform,
            overlay_cursor,
            buffer_size,
            translate_and_scale(
                output_space_damage,
                output_space_area,
                {{}, buffer_size}));
        break;

    case DamageAmount::full:
        current_frame.value().capture_frame(
            output_space_area,
            transform,
            overlay_cursor,
            buffer_size,
            {{}, buffer_size});
        break;
    }

    damage_amount = DamageAmount::none;
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
        new ExtImageCopyCaptureFrameV1{new_resource, this, ctx});
}

mf::ExtImageCopyCaptureFrameV1::ExtImageCopyCaptureFrameV1(
    wl_resource *resource,
    ExtImageCopyCaptureSessionV1* session,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx)
    : wayland::ImageCopyCaptureFrameV1{resource, Version<1>()},
      session{session},
      ctx{ctx}
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
    return capture_has_been_called;
}

void mf::ExtImageCopyCaptureFrameV1::capture_frame(
    geom::Rectangle output_space_area,
    glm::mat2 transform,
    bool overlay_cursor,
    geom::Size buffer_size,
    geom::Rectangle buffer_space_damage)
{
    if (!session)
    {
        send_failed_event(FailureReason::stopped);
        return;
    }

    auto shm_buffer = dynamic_cast<ShmBuffer*>(wayland::as_nullable_ptr(target));
    if (!shm_buffer)
    {
        send_failed_event(FailureReason::buffer_constraints);
        return;
    }
    auto shm_data = shm_buffer->data();
    if (shm_data->format() != mir_pixel_format_argb_8888 ||
        shm_data->size() != buffer_size)
    {
        send_failed_event(FailureReason::buffer_constraints);
        return;
    }

    // TODO: union buffer_space_damage with frame_damage to determine
    // region to copy.

    session.value().screen_shooter->capture(
        std::move(shm_data), output_space_area, transform, overlay_cursor,
        [executor=ctx->wayland_executor, buffer_space_damage, self=wayland::make_weak(this)](std::optional<time::Timestamp> captured_time)
        {
            executor->spawn([self, captured_time, buffer_space_damage]()
                {
                    if (self)
                    {
                        self.value().report_result(captured_time, buffer_space_damage);
                    }
                });
        });
    frame_damage = geom::Rectangle{};
}

void mf::ExtImageCopyCaptureFrameV1::report_result(
    std::optional<time::Timestamp> captured_time,
    geom::Rectangle buffer_space_damage)
{
    if (!captured_time)
    {
        send_failed_event(FailureReason::unknown);
        return;
    }
    send_damage_event(buffer_space_damage.left().as_int(),
                      buffer_space_damage.top().as_int(),
                      buffer_space_damage.size.width.as_int(),
                      buffer_space_damage.size.height.as_int());

    WaylandTimespec const timespec{captured_time.value()};
    send_presentation_time_event(
        timespec.tv_sec_hi,
        timespec.tv_sec_lo,
        timespec.tv_nsec);
    send_ready_event();
}
