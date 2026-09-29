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

#include "ext_foreign_buffer_v1.h"
#include "ext_image_capture_v1.h"
#include "wl_surface.h"

#include <mir/graphics/buffer.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/wayland/protocol_error.h>

#include <algorithm>

#include <vector>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace mw = mir::wayland;
namespace geom = mir::geometry;

namespace
{
auto const foreign_buffer_format = mir_pixel_format_argb_8888;

/// Buffers we hand to a client are sampled by the compositor while the next update is being
/// rendered, so compositing must never target a buffer that anything else still references.
class BufferRotation
{
public:
    explicit BufferRotation(std::shared_ptr<mg::GraphicBufferAllocator> allocator) :
        allocator{std::move(allocator)}
    {
    }

    auto next(geom::Size size) -> std::shared_ptr<mg::Buffer>
    {
        if (size != current_size)
        {
            buffers.clear();
            current_size = size;
        }

        // A slot we are the sole owner of is neither published nor being sampled.
        auto const free_slot = std::ranges::find_if(buffers, [](auto const& b) { return b.use_count() == 1; });
        if (free_slot != buffers.end())
        {
            return *free_slot;
        }

        auto buffer = allocator->alloc_software_buffer(size, foreign_buffer_format);
        if (buffers.size() < max_buffers)
        {
            buffers.push_back(buffer);
        }
        return buffer;
    }

private:
    static size_t constexpr max_buffers{4};

    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    geom::Size current_size;
};

class ExtForeignBufferManagerV1Global : public mw::ExtForeignBufferManagerV1::Global
{
public:
    ExtForeignBufferManagerV1Global(
        wl_display* display,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
};

class ExtForeignBufferManagerV1 : public mw::ExtForeignBufferManagerV1
{
public:
    ExtForeignBufferManagerV1(
        wl_resource* resource,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator) :
        mw::ExtForeignBufferManagerV1(resource, Version<1>()),
        allocator{allocator}
    {
    }

private:
    void get_session(wl_resource* session, wl_resource* source) override;

    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
};

/// The wl_buffer handed to the client. It outlives the session that created it, so it owns the
/// last content it was given rather than reaching back into the session.
class ForeignBuffer : public mf::ExtForeignBufferV1
{
public:
    explicit ForeignBuffer(wl_resource* resource) : mf::ExtForeignBufferV1(resource, Version<1>()) {}

    auto content() const -> std::shared_ptr<mg::Buffer> override { return current; }

    void add_consumer(mf::WlSurface& surface) override
    {
        auto const weak_surface = mw::make_weak(&surface);
        if (std::ranges::find(consumers, weak_surface) == consumers.end())
        {
            consumers.push_back(weak_surface);
        }
    }

    void set_content(std::shared_ptr<mg::Buffer> buffer)
    {
        current = std::move(buffer);

        std::erase_if(consumers, [](auto const& consumer) { return !consumer; });
        for (auto const& consumer : consumers)
        {
            consumer.value().foreign_buffer_updated(*this, current);
        }
    }

private:
    std::shared_ptr<mg::Buffer> current;
    std::vector<mw::Weak<mf::WlSurface>> consumers;
};

class ExtForeignBufferSessionV1 : public mw::ExtForeignBufferSessionV1, public mf::ExtImageCopyBackendSession
{
public:
    ExtForeignBufferSessionV1(
        wl_resource* resource,
        mf::ExtImageCopyBackendFactory const& factory,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator) :
        mw::ExtForeignBufferSessionV1(resource, Version<1>()),
        rotation{allocator},
        backend{factory(this, false)}
    {
    }

    void get_buffer(wl_resource* buffer) override;

    void destroy() override {}

    void maybe_capture_frame() override;

    void set_buffer_constraints(geom::Size const& size) override;

    void set_stopped() override;

private:
    void composite_frame();
    void publish(std::shared_ptr<mg::Buffer> buffer);

    BufferRotation rotation;
    mw::Weak<ForeignBuffer> foreign_buffer;
    /// Retained on the session so that a buffer requested after the source stopped (or after the
    /// client destroyed a previous one) still resolves to the last content we captured.
    std::shared_ptr<mg::Buffer> last_content;
    geom::Size source_size;
    /// The buffer size most recently described to the client by a size event. Empty until a size
    /// event has been sent for the active buffer.
    geom::Size reported_buffer_size;
    bool capture_in_flight{false};
    bool invalidated{false};
    bool stopped{false};
    std::shared_ptr<mf::ExtImageCopyBackend> backend;
};

void ExtForeignBufferSessionV1::get_buffer(wl_resource* buffer)
{
    if (foreign_buffer)
    {
        throw mw::ProtocolError{
            resource,
            Error::buffer_active,
            "get_buffer requested while a buffer is still active for this session"};
    }

    auto const created = new ForeignBuffer{buffer};
    foreign_buffer = mw::make_weak(created);
    invalidated = false;
    reported_buffer_size = {};

    // The difference between the [ExtForeignBufferSessionV1] and the [mf::ExtImageCopyCaptureSessionV1]
    // is that this session will attempt to return a zero-copy, opaque buffer to the client. A
    // zero-copy buffer is only available when the source is already backed by a single buffer, so
    // anything else (an output, or a toplevel with subsurfaces) is composited into a buffer we
    // allocate ourselves. Either way the client only ever receives an opaque handle.
    maybe_capture_frame();

    auto const buffer_size = last_content ? last_content->size() : source_size;
    reported_buffer_size = buffer_size;
    send_size_event(
        buffer_size.width.as_uint32_t(),
        buffer_size.height.as_uint32_t(),
        source_size.width.as_uint32_t(),
        source_size.height.as_uint32_t());

    if (last_content)
    {
        created->set_content(last_content);
    }
}

void ExtForeignBufferSessionV1::maybe_capture_frame()
{
    if (stopped)
    {
        return;
    }

    // Passing `this` as the consumer id keeps the source stream's release bookkeeping
    // separate from any other consumer of the same surface.
    if (auto const content = backend->acquire_content(this))
    {
        publish(content->buffer);
        return;
    }

    composite_frame();
}

void ExtForeignBufferSessionV1::composite_frame()
{
    // TODO: guard against the capture/damage feedback loop for output sources. Compositing an
    // output renders every surface on it, including the consumer displaying this buffer, so
    // publishing new content damages the scene and immediately schedules another capture. The
    // capture is asynchronous, so this is a busy loop rather than stack recursion, but it is still
    // unbounded. miral::RenderSceneIntoSurface solves the same problem by having the target
    // surface return no renderables to its own screen shooter (see
    // src/miral/render_scene_into_surface.cpp), which needs an equivalent per-consumer exclusion
    // plumbed from WlSurface down to scene::Surface::generate_renderables.

    // One capture at a time: a capture completes asynchronously, and damage may well arrive
    // faster than we can service it.
    if (capture_in_flight || !backend->has_damage() || source_size == geom::Size{})
    {
        return;
    }

    auto const target = rotation.next(source_size);
    capture_in_flight = true;
    backend->begin_capture(
        mrs::as_write_mappable(target),
        {{}, source_size},
        [weak_self = mw::make_weak(this), target](mf::ExtImageCopyBackend::CaptureResult const& result)
        {
            if (!weak_self)
            {
                return;
            }

            auto& self = weak_self.value();
            self.capture_in_flight = false;
            if (result)
            {
                self.publish(target);
            }

            // Damage that arrived while the capture was running was skipped, and nothing else
            // will come back to collect it.
            self.maybe_capture_frame();
        });
}

void ExtForeignBufferSessionV1::publish(std::shared_ptr<mg::Buffer> buffer)
{
    if (!buffer)
    {
        return;
    }

    last_content = std::move(buffer);

    if (!foreign_buffer || reported_buffer_size == geom::Size{})
    {
        // Nothing has been described to the client yet; get_buffer() publishes the content once
        // it has sent the matching size event.
        return;
    }

    if (last_content->size() != reported_buffer_size)
    {
        // The size event the client is holding no longer describes what we are producing, so it
        // has to request a new buffer before we can keep it up to date.
        if (!invalidated)
        {
            invalidated = true;
            send_buffer_invalidated_event();
        }
        return;
    }

    foreign_buffer.value().set_content(last_content);
}

void ExtForeignBufferSessionV1::set_buffer_constraints(geom::Size const& size)
{
    if (size == source_size)
    {
        return;
    }

    source_size = size;
    if (foreign_buffer && !invalidated)
    {
        invalidated = true;
        send_buffer_invalidated_event();
    }
}

void ExtForeignBufferSessionV1::set_stopped()
{
    if (stopped)
    {
        return;
    }

    // The buffer stays valid, resolving to whatever content it last held.
    stopped = true;
    send_stopped_event();
}

ExtForeignBufferManagerV1Global::ExtForeignBufferManagerV1Global(
    wl_display* display,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator) :
    Global(display, Version<1>()),
    allocator{allocator}
{
}

void ExtForeignBufferManagerV1Global::bind(wl_resource* new_resource)
{
    new ExtForeignBufferManagerV1{new_resource, allocator};
}

void ExtForeignBufferManagerV1::get_session(wl_resource* session, wl_resource* source)
{
    auto const source_instance = mf::ExtImageCaptureSourceV1::from_or_throw(source);
    new ExtForeignBufferSessionV1{session, source_instance->backend_factory, allocator};
}
}

auto mf::ExtForeignBufferV1::from(wl_resource* resource) -> ExtForeignBufferV1*
{
    if (auto const buffer = mw::Buffer::from(resource))
    {
        return dynamic_cast<ExtForeignBufferV1*>(buffer);
    }
    return nullptr;
}

auto mf::create_ext_foreign_buffer_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const&,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
    -> std::shared_ptr<mw::ExtForeignBufferManagerV1::Global>
{
    return std::make_shared<ExtForeignBufferManagerV1Global>(display, allocator);
}
