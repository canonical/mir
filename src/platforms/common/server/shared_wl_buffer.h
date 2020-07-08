//
// Created by chris on 9/6/20.
//

#ifndef MIR_SHARED_WL_BUFFER_H
#define MIR_SHARED_WL_BUFFER_H

#include <memory>
#include <mutex>

struct wl_resource;
struct wl_listener;

namespace mir
{
class Executor;

namespace graphics
{
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
 */
class SharedWlBuffer
{
public:
    SharedWlBuffer(wl_resource* buffer, std::shared_ptr<mir::Executor> wayland_executor);
    ~SharedWlBuffer() noexcept;

    SharedWlBuffer(SharedWlBuffer&& from) noexcept;

    class LockedHandle
    {
    public:
        LockedHandle(LockedHandle&& from) noexcept;
        ~LockedHandle() noexcept;

        operator wl_resource*() const;
        operator bool() const;
    private:
        std::unique_lock<std::mutex> lock;
        wl_resource* locked_buffer;

        friend class SharedWlBuffer;
        LockedHandle(wl_resource* buffer, std::unique_lock<std::mutex>&& lock);
        LockedHandle();
    };

    auto lock() const -> LockedHandle;
private:
    struct WlResource;
    WlResource* resource;

    static void on_buffer_destroyed(wl_listener* listener, void*);
    static auto resource_for_buffer(
        wl_resource* buffer,
        std::shared_ptr<mir::Executor> wayland_executor) -> WlResource*;
};

}
}


#endif //MIR_SHARED_WL_BUFFER_H
