//
// Created by chris on 9/6/20.
//

#include "mir/graphics/shared_wl_buffer.h"
#include "mir/executor.h"

#include <wayland-server.h>

#include <atomic>
#include <cassert>

namespace mg = mir::graphics;

struct mg::SharedWlBuffer::WlResource
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

mg::SharedWlBuffer::SharedWlBuffer(
    wl_resource* buffer,
    std::shared_ptr<mir::Executor> wayland_executor)
    : resource{resource_for_buffer(buffer, std::move(wayland_executor))}
{
    /* Because we are on the Wayland event loop, this is not racy:
     * The resource destructor runs only on the Wayland loop, so cannot have run
     * between us acquiring the WlResource* and this increment of the use-count.
     */
    resource->use_count++;
}

mg::SharedWlBuffer::~SharedWlBuffer() noexcept
{
    // Release our shared ownership (possibly sending WL_BUFFER_RELEASE).
    if (resource)
    {
        resource->put();
    }
}

mg::SharedWlBuffer::SharedWlBuffer(SharedWlBuffer&& from) noexcept
        : resource{from.resource}
{
    from.resource = nullptr;
}

mg::SharedWlBuffer::LockedHandle::LockedHandle(LockedHandle&& from) noexcept
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

mg::SharedWlBuffer::LockedHandle::~LockedHandle() noexcept
{
    if (lock.owns_lock())
        lock.unlock();
}

mg::SharedWlBuffer::LockedHandle::operator wl_resource*() const
{
    return locked_buffer;
}

mg::SharedWlBuffer::LockedHandle::operator bool() const
{
    return locked_buffer;
}

mg::SharedWlBuffer::LockedHandle::LockedHandle(wl_resource* buffer, std::unique_lock<std::mutex>&& lock)
    : lock{std::move(lock)},
      locked_buffer{buffer}
{
}

mg::SharedWlBuffer::LockedHandle::LockedHandle()
    : locked_buffer{nullptr}
{
}

auto mg::SharedWlBuffer::lock() const -> LockedHandle
{
    std::unique_lock<std::mutex> lock{resource->mutex};
    if (resource->buffer)
    {
        return LockedHandle{resource->buffer, std::move(lock)};
    }
    return LockedHandle{};
}

void mg::SharedWlBuffer::on_buffer_destroyed(wl_listener* listener, void*)
{
    WlResource* resource;
    resource = wl_container_of(listener, resource, destruction_listener);

    {
        std::lock_guard<std::mutex> lock{resource->mutex};
        resource->buffer = nullptr;
    }
    // Release the wl_resource's ownership
    resource->put();
}

auto mg::SharedWlBuffer::resource_for_buffer(
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

