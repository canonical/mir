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
#include "wayland_executor.h"
#include "wl_surface.h"

#include <mir/graphics/buffer.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/wayland/protocol_error.h>

#include <algorithm>
#include <optional>
#include <vector>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace mw = mir::wayland;
namespace geom = mir::geometry;

namespace
{
auto const foreign_buffer_format = mir_pixel_format_argb_8888;

class BufferPool
{
public:
    explicit BufferPool(std::shared_ptr<mg::GraphicBufferAllocator> allocator) :
        allocator{std::move(allocator)}
    {
        buffers.reserve(max_buffers);
    }

    auto next(geom::Size size) -> std::shared_ptr<mg::Buffer>
    {
        if (size != current_size)
        {
            buffers.clear();
            current_size = size;
        }

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

class ExtForeignBufferManagerV1Global : public mw::ForeignBufferManagerV1::Global
{
public:
    ExtForeignBufferManagerV1Global(
        wl_display* display,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<mf::WaylandExecutor> const& executor);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mf::WaylandExecutor> executor;
};

class ExtForeignBufferManagerV1 : public mw::ForeignBufferManagerV1
{
public:
    ExtForeignBufferManagerV1(
        wl_resource* resource,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<mf::WaylandExecutor> const& executor) :
        mw::ForeignBufferManagerV1(resource, Version<1>()),
        allocator{allocator},
        executor{executor}
    {
    }

private:
    void get_session(wl_resource* session, wl_resource* source) override;

    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mf::WaylandExecutor> executor;
};

class ExtForeignBufferSessionV1 : public mw::ForeignBufferSessionV1, public mf::ExtImageCopyBackendSession
{
public:
    ExtForeignBufferSessionV1(
        wl_resource* resource,
        mf::ExtImageCopyBackendFactory const& factory,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<mf::WaylandExecutor> const& executor) :
        mw::ForeignBufferSessionV1(resource, Version<1>()),
        pool{allocator},
        backend{factory(this, false)},
        executor{executor}
    {
    }

    void get_buffer(wl_resource* buffer) override;

    void maybe_capture_frame() override;

    void set_buffer_constraints(geom::Size const& buffer_size, geom::Size const& logical_size) override;

    void set_stopped() override;

private:
    /// The size of the capture source as reported by the backend.
    struct SourceSize
    {
        /// The pixel size that composited captures are rendered at.
        geom::Size buffer_size;
        /// The size sent as the logical size in the size event.
        geom::Size logical_size;

        auto operator==(SourceSize const&) const -> bool = default;
    };

    /// The metadata described by the size event.
    struct FrameMetadata
    {
        geom::Size buffer_size;
        geom::Size logical_size;

        auto operator==(FrameMetadata const&) const -> bool = default;
    };

    struct Frame
    {
        std::shared_ptr<mg::Buffer> buffer;
        /// The source size at the time the frame was captured.
        SourceSize source_size;

        auto metadata() const -> FrameMetadata { return {buffer->size(), source_size.logical_size}; }
    };

    void capture_frame();
    void frame_captured(Frame frame);
    void publish(Frame const& frame);

    BufferPool pool;
    mw::Weak<mf::ExtForeignBufferV1> foreign_buffer;
    /// The most recent successfully captured frame. It outlives any single foreign buffer, so
    /// that a buffer requested while the source is unchanged, or after it has stopped, can be
    /// given the last content without waiting for damage that may never come.
    std::optional<Frame> latest_frame;
    /// The metadata sent in the size event for the current foreign buffer. Unset until the
    /// first frame is published after get_buffer(). Any later frame with different metadata
    /// invalidates the buffer, so the client has to request a new one.
    std::optional<FrameMetadata> reported_metadata;
    /// Empty until the backend reports it, and capture is skipped while it is empty.
    SourceSize source_size;
    bool capture_in_flight{false};
    bool invalidated{false};
    bool stopped{false};
    std::shared_ptr<mf::ExtImageCopyBackend> backend;
    std::shared_ptr<mf::WaylandExecutor> executor;
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

    auto const created = new mf::ExtForeignBufferV1{buffer};
    invalidated = false;
    reported_metadata.reset();
    foreign_buffer = mw::make_weak(created);
    if (latest_frame)
    {
        publish(*latest_frame);
    }
    capture_frame();
}

void ExtForeignBufferSessionV1::maybe_capture_frame()
{
    // TODO: We can probably get around this, but it is best saved for a later refactor:
    //
    // Backends report their initial damage from their own constructor, which runs before our
    // `backend` member has been assigned. We can safely defer this until afterward, because with
    // no buffer, there is no consumer to capture for, and the damage the backend is holding is
    // serviced by the next get_buffer(). We can probably refactor this in the future to split up
    // the responsibilities: of the backend: one object handles notifying us to update, the other
    // provides the facilities to act on those updates (e.g. by calling `begin_capture`).
    if (!foreign_buffer)
    {
        return;
    }

    capture_frame();
}

void ExtForeignBufferSessionV1::capture_frame()
{
    if (stopped || capture_in_flight || !backend->has_damage() || source_size.buffer_size == geom::Size{})
    {
        return;
    }

    // First, try to capture the frame via a zero-copy buffer.
    //
    // Passing `this` as the consumer id keeps the source stream's release bookkeeping
    // separate from any other consumer of the same surface.
    if (auto content = backend->acquire_content(this))
    {
        frame_captured({std::move(content), source_size});
        return;
    }

    // If acquiring a zero-copy buffer fails, then composite the content into a buffer on the side.
    // It only replaces `latest_frame` once the capture succeeds.
    auto const captured_size = source_size;
    auto const target = pool.next(captured_size.buffer_size);
    capture_in_flight = true;
    backend->begin_capture(
        mrs::as_write_mappable(target),
        {{}, captured_size.buffer_size},
        [weak_self = mw::make_weak(this), target, captured_size](
            mf::ExtImageCopyBackend::CaptureResult const& result)
        {
            if (!weak_self)
            {
                return;
            }

            auto& self = weak_self.value();
            self.capture_in_flight = false;

            if (result)
            {
                self.frame_captured({target, captured_size});
            }

            // Damage that arrived while the capture was running was skipped, and nothing else
            // will come back to collect it. We spawn this work on an executor so that it happens
            // after `begin_capture` has time to return and potentially set the current damage
            // to zero.
            self.executor->spawn([weak_self]
            {
                if (!weak_self)
                {
                    return;
                }

                auto& self = weak_self.value();
                self.maybe_capture_frame();
            });
        });
}

void ExtForeignBufferSessionV1::frame_captured(Frame frame)
{
    latest_frame = std::move(frame);
    publish(*latest_frame);
}

void ExtForeignBufferSessionV1::publish(Frame const& frame)
{
    if (!foreign_buffer)
    {
        return;
    }

    auto const metadata = frame.metadata();
    if (!reported_metadata)
    {
        // A frame captured before the source resized would be invalidated as soon as it was
        // described, so wait for the capture at the new size instead. Once stopped, no such
        // capture will come, and the last content is all there is.
        if (!stopped && frame.source_size != source_size)
        {
            return;
        }

        reported_metadata = metadata;
        send_size_event(
            metadata.buffer_size.width.as_uint32_t(),
            metadata.buffer_size.height.as_uint32_t(),
            metadata.logical_size.width.as_uint32_t(),
            metadata.logical_size.height.as_uint32_t());
    }
    else if (metadata != reported_metadata)
    {
        if (!invalidated)
        {
            invalidated = true;
            send_buffer_invalidated_event();
        }
        return;
    }

    foreign_buffer.value().set_content(frame.buffer);
}

void ExtForeignBufferSessionV1::set_buffer_constraints(geom::Size const& buffer_size, geom::Size const& logical_size)
{
    SourceSize const size{buffer_size, logical_size};
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
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mf::WaylandExecutor> const& executor) :
    Global(display, Version<1>()),
    allocator{allocator},
    executor{executor}
{
}

void ExtForeignBufferManagerV1Global::bind(wl_resource* new_resource)
{
    new ExtForeignBufferManagerV1{new_resource, allocator, executor};
}

void ExtForeignBufferManagerV1::get_session(wl_resource* session, wl_resource* source)
{
    auto const source_instance = mf::ExtImageCaptureSourceV1::from_or_throw(source);
    new ExtForeignBufferSessionV1{session, source_instance->backend_factory, allocator, executor};
}
}

mf::ExtForeignBufferV1::ExtForeignBufferV1(wl_resource* resource) : mw::Buffer(resource, Version<1>())
{
}

auto mf::ExtForeignBufferV1::from(wl_resource* resource) -> ExtForeignBufferV1*
{
    if (auto const buffer = mw::Buffer::from(resource))
    {
        return dynamic_cast<ExtForeignBufferV1*>(buffer);
    }
    return nullptr;
}

auto mf::ExtForeignBufferV1::content() const -> std::shared_ptr<mg::Buffer>
{
    return current;
}

void mf::ExtForeignBufferV1::add_listener(WlSurface& surface)
{
    auto const weak_surface = mw::make_weak(&surface);
    if (std::ranges::find(consumers, weak_surface) == consumers.end())
    {
        consumers.push_back(weak_surface);
    }
}

void mf::ExtForeignBufferV1::remove_listener(WlSurface& surface)
{
    auto const weak_surface = mw::make_weak(&surface);
    std::erase_if(consumers, [&weak_surface](auto const& consumer) { return weak_surface == consumer; });
}

void mf::ExtForeignBufferV1::set_content(std::shared_ptr<mg::Buffer> buffer)
{
    current = std::move(buffer);

    std::erase_if(consumers, [](auto const& consumer) { return !consumer; });
    for (auto const& consumer : consumers)
    {
        consumer.value().advise_foreign_buffer_content_changed(current);
    }
}

auto mf::create_ext_foreign_buffer_manager_v1(
    wl_display* display,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<WaylandExecutor> const& executor)
    -> std::shared_ptr<mw::ForeignBufferManagerV1::Global>
{
    return std::make_shared<ExtForeignBufferManagerV1Global>(display, allocator, executor);
}
