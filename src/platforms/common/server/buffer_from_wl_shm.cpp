/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "buffer_from_wl_shm.h"
#include "shm_buffer.h"

#include "mir/graphics/egl_context_executor.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/executor.h"

#define MIR_LOG_COMPONENT "wayland-gfx-helpers"
#include "mir/log.h"

#include <boost/throw_exception.hpp>
#include <mutex>
#include <atomic>

#include <GLES2/gl2.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <cassert>
#include <cstring>

namespace mg = mir::graphics;
namespace mgc = mir::graphics::common;

namespace mir
{
namespace graphics
{
class NativeBuffer;
}
}

MirPixelFormat wl_format_to_mir_format(uint32_t format)
{
    switch (format)
    {
    case WL_SHM_FORMAT_ARGB8888:
        return mir_pixel_format_argb_8888;
    case WL_SHM_FORMAT_XRGB8888:
        return mir_pixel_format_xrgb_8888;
    case WL_SHM_FORMAT_RGBA4444:
        return mir_pixel_format_rgba_4444;
    case WL_SHM_FORMAT_RGBA5551:
        return mir_pixel_format_rgba_5551;
    case WL_SHM_FORMAT_RGB565:
        return mir_pixel_format_rgb_565;
    case WL_SHM_FORMAT_RGB888:
        return mir_pixel_format_rgb_888;
    case WL_SHM_FORMAT_BGR888:
        return mir_pixel_format_bgr_888;
    case WL_SHM_FORMAT_XBGR8888:
        return mir_pixel_format_xbgr_8888;
    case WL_SHM_FORMAT_ABGR8888:
        return mir_pixel_format_abgr_8888;
    default:
        return mir_pixel_format_invalid;
    }
}

/**
 * A shared-pointer-like handle to a wl_buffer
 *
 * We need three things from a wl_buffer:
 * 1. we need to keep some handle around for Mir objects, which might possibly
 *    outlive the Wayland resource.
 * 2. we need to send a WL_BUFFER_RELEASE when the *last* MirBuffer referencing
 *    the wl_buffer goes away. `wl_buffers` *can* be submitted to different
 *    `wl_surface`s; these want to be represented as different MirBuffers, because
 *    they're going in different BufferStreams, have distinct frame callbacks, and so on.
 * 3. we need a way to prevent the wl_buffer being destroyed while a different thread
 *    is operating on it.
 *
 * SharedWlBuffer provides this behaviour.
 *
 * Theory of operation:
 * In standard fashion we hang a WlResource struct off the destruction listener
 * of the wl_resource. The WlResource is manually reference-counted; we count
 * one reference for the wl_resource, and count one reference per external referrer.
 *
 * When the reference count hits 1 we queue a work item on the Wayland event loop
 * to send WL_BUFFER_RELEASE (if the wl_resource is still live).
 *
 * When the reference count hits 0 we destroy the WlResource; the reference count
 * can only hit 0 if the wl_buffer itself has been destroyed, so there's no way for
 * other code to acquire a reference to the WlResource.
 */
class SharedWlBuffer
{
public:
    /*
     * NOTE: This must be called on the Wayland event loop
     */
    SharedWlBuffer(wl_resource* buffer, std::shared_ptr<mir::Executor> wayland_executor)
        : resource{resource_for_buffer(buffer, std::move(wayland_executor))}
    {
        /* Because we are on the Wayland event loop, this is not racy:
         * The resource destructor runs only on the Wayland loop, so cannot have run
         * between us acquiring the WlResource* and this increment of the use-count.
         */
        resource->use_count++;
    }

    ~SharedWlBuffer()
    {
        // Release our shared ownership (possibly sending WL_BUFFER_RELEASE).
        if (resource)
        {
            resource->put();
        }
    }

    SharedWlBuffer(SharedWlBuffer&& from) noexcept
        : resource{from.resource}
    {
        from.resource = nullptr;
    }

    class LockedHandle
    {
    public:
        LockedHandle(LockedHandle&& from) noexcept
            : lock{std::move(from.lock)},
              locked_buffer{from.locked_buffer}
        {
            /* We could technically use the default move constructor,
             * but that would leave us with a moved-from object which
             * is not locked but evaluates to a (maybe!) valid wl_resource*.
             *
             * It doesn't hurt to explicitly kill the moved-from handle.
             */
            from.locked_buffer = nullptr;
        }

        ~LockedHandle()
        {
            if (lock.owns_lock())
                lock.unlock();
        }

        operator wl_resource*() const
        {
            return locked_buffer;
        }

        operator bool() const
        {
            return locked_buffer;
        }

    private:
        std::unique_lock<std::mutex> lock;
        wl_resource* locked_buffer;

        friend class SharedWlBuffer;
        LockedHandle(wl_resource* buffer, std::unique_lock<std::mutex>&& lock)
            : lock{std::move(lock)},
              locked_buffer{buffer}
        {
        }
        LockedHandle()
            : locked_buffer{nullptr}
        {
        }
    };

    LockedHandle lock() const
    {
        std::unique_lock lock{resource->mutex};
        if (resource->buffer)
        {
            return LockedHandle{resource->buffer, std::move(lock)};
        }
        return LockedHandle{};
    }
private:
    struct WlResource
    {
        WlResource(wl_resource* buffer, std::shared_ptr<mir::Executor> wayland_executor)
            : use_count{1},     // We start off owned only by the wl_resource.
              buffer{buffer},
              wayland_executor{std::move(wayland_executor)}
        {
            destruction_listener.notify = &on_buffer_destroyed;
            wl_resource_add_destroy_listener(buffer, &destruction_listener);
        }

        void put()
        {
            auto const prev_use_count = use_count.fetch_sub(1);
            if (prev_use_count == 2)
            {
                /*
                 * use_count is now 1 (because it used to be 2, and we subtracted 1).
                 *
                 * This means that it's time to send WL_BUFFER_RELEASE (if the Wayland
                 * object hasn't already been destroyed, of course).
                 *
                 * However!
                 * We don't want to accidentally free the WlResource before processing
                 * this, so we re-increment use_count.
                 */
                use_count++;
                wayland_executor->spawn(
                    [this]()
                    {
                        /* No need to lock buffer; it can only be destroyed on the Wayland
                         * event loop, which is where we are.
                         */
                        if (buffer)
                        {
                            wl_resource_queue_event(buffer, WL_BUFFER_RELEASE);
                        }

                        /* We now need to release our temporary use-count.
                         *
                         * We've just sent a WL_BUFFER_RELEASE, so all we need to care
                         * about is cleaning up if this reference was the last one.
                         */
                        if (use_count.fetch_sub(1) == 1)
                        {
                            assert(buffer == nullptr);
                            delete this;
                        }
                    });
            }
            else if (prev_use_count == 1)
            {
                /*
                 * use_count is now 0; nothing else can get a reference to use,
                 * time to free!
                 */
                delete this;
            }
        }

        std::atomic<int> use_count;
        std::mutex mutex;
        wl_resource* buffer;
        std::shared_ptr<mir::Executor> const wayland_executor;
        wl_listener destruction_listener;
    };

    WlResource* resource;

    static void on_buffer_destroyed(wl_listener* listener, void*)
    {
        WlResource* resource;
        resource = wl_container_of(listener, resource, destruction_listener);

        {
            std::lock_guard lock{resource->mutex};
            resource->buffer = nullptr;
        }
        // Release the wl_resource's ownership
        resource->put();
    }

    static auto resource_for_buffer(
        wl_resource* buffer,
        std::shared_ptr<mir::Executor> wayland_executor) -> WlResource*
    {
        static_assert(
            std::is_standard_layout<WlResource>::value,
            "WlResource must be Standard Layout for wl_container_of to be defined behaviour");

        WlResource* resource;
        if (auto notifier = wl_resource_get_destroy_listener(buffer, &on_buffer_destroyed))
        {
            // We've seen this buffer before; use the existing WlResource wrapper
            resource = wl_container_of(notifier, resource, destruction_listener);

            /* The existing wrapper *must* have been associated with this buffer,
             * because we've pulled the reference from this buffer. But let's just be sure…
             */
            assert(resource->buffer == buffer);
        }
        else
        {
            // First time we've asked for a WlResource wrapper for this buffer
            resource = new WlResource{buffer, std::move(wayland_executor)};
        }
        return resource;
    }
};

class WlShmBuffer :
    public mg::common::ShmBuffer,
    public mir::renderer::software::RWMappableBuffer
{
public:
    WlShmBuffer(
        SharedWlBuffer buffer,
        std::shared_ptr<mgc::EGLContextExecutor> egl_delegate,
        mir::geometry::Size const& size,
        mir::geometry::Stride stride,
        MirPixelFormat format,
        std::function<void()>&& on_consumed)
        : ShmBuffer(size, format, egl_delegate),
          on_consumed{std::move(on_consumed)},
          buffer{std::move(buffer)},
          stride_{stride}
    {
        egl_delegate->spawn([this]()
        {
            ShmBuffer::bind();
            {
                std::lock_guard lock{upload_mutex};
                auto const mapping = map_readable();
                upload_to_texture(mapping->data(), mapping->stride());
                uploaded = true;
            }
            upload_cv.notify_one();
        });
    }

    ~WlShmBuffer()
    {
        wait_for_upload();
    }

    void bind() override
    {
        wait_for_upload();
        ShmBuffer::bind();
        notify_consumed();
    }

    void wait_for_upload() const
    {
        std::unique_lock lock{upload_mutex};
        upload_cv.wait(lock, [&] { return uploaded; });
    }

    auto map_readable() -> std::unique_ptr<mir::renderer::software::Mapping<unsigned char const>> override
    {
        return map_generic<unsigned char const>();
    }

    auto map_writeable() -> std::unique_ptr<mir::renderer::software::Mapping<unsigned char>> override
    {
        return map_generic<unsigned char>();
    }

    auto map_rw() -> std::unique_ptr<mir::renderer::software::Mapping<unsigned char>> override
    {
        return map_generic<unsigned char>();
    }

    template<typename T>
    auto map_generic() -> std::unique_ptr<mir::renderer::software::Mapping<T>>
    {
        class Mapping : public mir::renderer::software::Mapping<T>
        {
        public:
            Mapping(
                SharedWlBuffer::LockedHandle&& buffer,
                WlShmBuffer* parent)
                : buffer{std::move(buffer)},
                  shm_buffer{wl_shm_buffer_get(this->buffer)},
                  parent{parent}
            {
                wl_shm_buffer_begin_access(shm_buffer);
            }

            ~Mapping()
            {
                wl_shm_buffer_end_access(shm_buffer);
            }

            auto format() const -> MirPixelFormat override
            {
                return parent->pixel_format();
            }

            auto stride() const -> mir::geometry::Stride override
            {
                return parent->stride_;
            }

            auto size() const -> mir::geometry::Size override
            {
                return parent->size();
            }

            auto data() -> T* override
            {
                return static_cast<T*>(wl_shm_buffer_get_data(shm_buffer));
            }

            auto len() const -> size_t override
            {
                return stride().as_uint32_t() * size().height.as_uint32_t();
            }

        private:
            SharedWlBuffer::LockedHandle const buffer;
            wl_shm_buffer* const shm_buffer;
            WlShmBuffer* const parent;
        };

        if (auto locked_buffer = buffer.lock())
        {
            return std::make_unique<Mapping>(std::move(locked_buffer), this);
        }
        else
        {
            mir::log_debug("Wayland buffer destroyed before use; rendering will be incomplete");
            class FallbackMapping : public mir::renderer::software::Mapping<T>
            {
            public:
                FallbackMapping(
                    MirPixelFormat format,
                    mir::geometry::Size size,
                    mir::geometry::Stride stride)
                    : buffer{std::make_unique<unsigned char[]>(size.height.as_uint32_t() * stride.as_uint32_t())},
                      format_{format},
                      size_{size},
                      stride_{stride}
                {
                    ::memset(buffer.get(), 0, len());
                }

                auto format() const -> MirPixelFormat override
                {
                    return format_;
                }

                auto stride() const -> mir::geometry::Stride override
                {
                    return stride_;
                }

                auto size() const -> mir::geometry::Size override
                {
                    return size_;
                }

                auto data() -> T* override
                {
                    return buffer.get();
                }

                auto len() const -> size_t override
                {
                    return size().height.as_uint32_t() * stride().as_uint32_t();
                }
            private:
                std::unique_ptr<unsigned char[]> const buffer;
                MirPixelFormat format_;
                mir::geometry::Size size_;
                mir::geometry::Stride stride_;
            };

            return std::make_unique<FallbackMapping>(pixel_format(), size(), stride_);
        }
    }

private:
    void notify_consumed()
    {
        bool has_not_been_consumed{false};
        if (consumed.compare_exchange_strong(has_not_been_consumed, true))
        {
            on_consumed();
        }
    }

    std::atomic<bool> consumed{false};
    std::function<void()> on_consumed;

    std::mutex mutable upload_mutex;
    std::condition_variable mutable upload_cv;
    bool uploaded{false};
    SharedWlBuffer const buffer;
    mir::geometry::Stride const stride_;
};

auto mg::wayland::buffer_from_wl_shm(
    wl_resource* buffer,
    std::shared_ptr<Executor> executor,
    std::shared_ptr<common::EGLContextExecutor> egl_delegate,
    std::function<void()>&& on_consumed) -> std::shared_ptr<Buffer>
{
    auto const shm_buffer = wl_shm_buffer_get(buffer);
    if (!shm_buffer)
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"Attempt to import a non-SHM buffer as a SHM buffer"}));
    }
    return std::make_shared<WlShmBuffer>(
        SharedWlBuffer{buffer, std::move(executor)},
        std::move(egl_delegate),
        mir::geometry::Size{
            wl_shm_buffer_get_width(shm_buffer),
            wl_shm_buffer_get_height(shm_buffer)
        },
        mir::geometry::Stride{wl_shm_buffer_get_stride(shm_buffer)},
        wl_format_to_mir_format(wl_shm_buffer_get_format(shm_buffer)),
        std::move(on_consumed));
}
