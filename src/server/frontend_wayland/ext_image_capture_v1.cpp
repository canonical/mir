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

#include "mir/wayland/weak.h"
#include "output_manager.h"

namespace mf = mir::frontend;

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
                            OutputInstance* output,
                            ExtOutputImageCaptureSourceManagerV1* manager);

    static ExtImageCaptureSourceV1* from_or_throw(wl_resource* resource);

private:
    // FIXME: currently this is only set up for output capture. It
    // will need to change to also cover toplevel capture.
    wayland::Weak<OutputInstance> const output;
    wayland::Weak<ExtOutputImageCaptureSourceManagerV1> const manager;
};


/* Image capture sessions */
struct ExtImageCaptureV1Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
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

class ExtImageCopyCaptureSessionV1
    : public wayland::ImageCopyCaptureSessionV1
{
public:
    ExtImageCopyCaptureSessionV1(wl_resource* resource, ExtImageCaptureSourceV1* source, uint32_t options, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void create_frame(wl_resource* new_resource) override;

    wayland::Weak<ExtImageCaptureSourceV1> const source;
    uint32_t const options;
    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

class ExtImageCopyCaptureFrameV1
    : public wayland::ImageCopyCaptureFrameV1
{
public:
    ExtImageCopyCaptureFrameV1(wl_resource* resource, ExtImageCopyCaptureSessionV1* session, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void attach_buffer(wl_resource* buffer) override;
    void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void capture() override;

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
    auto output_instance = OutputInstance::from(output);
    new ExtImageCaptureSourceV1{new_resource, output_instance, this};
}

mf::ExtImageCaptureSourceV1::ExtImageCaptureSourceV1(
    wl_resource* resource, OutputInstance* output,
    ExtOutputImageCaptureSourceManagerV1 *manager)
    : wayland::ImageCaptureSourceV1{resource, Version<1>()},
      output{output},
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
    std::shared_ptr<Executor> const& wayland_executor)
-> std::shared_ptr<wayland::ImageCopyCaptureManagerV1::Global>
{
    auto ctx = std::make_shared<ExtImageCaptureV1Ctx>(wayland_executor);
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
      source{source},
      options{options},
      ctx{ctx}
{
    // FIXME: send buffer format events
    send_done_event();
}

void mf::ExtImageCopyCaptureSessionV1::create_frame(
    [[maybe_unused]] wl_resource* new_resource)
{
    new ExtImageCopyCaptureFrameV1{new_resource, this, ctx};
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
    [[maybe_unused]] wl_resource* buffer)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("not implemented"));
}

void mf::ExtImageCopyCaptureFrameV1::damage_buffer(
    [[maybe_unused]] int32_t x,
    [[maybe_unused]] int32_t y,
    [[maybe_unused]] int32_t width,
    [[maybe_unused]] int32_t height)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("not implemented"));
}

void mf::ExtImageCopyCaptureFrameV1::capture()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("not implemented"));
}
