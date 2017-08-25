//
// Created by chris on 8/08/17.
//

#ifndef MIR_PLATFORM_GRAPHICS_WAYLAND_ALLOCATOR_H_
#define MIR_PLATFORM_GRAPHICS_WAYLAND_ALLOCATOR_H_

#include <vector>
#include <memory>

#include <wayland-server-core.h>

namespace mir
{
class Executor;

namespace graphics
{
class Buffer;

class WaylandAllocator
{
public:
    virtual ~WaylandAllocator() = default;

    virtual void bind_display(wl_display* display) = 0;
    virtual std::unique_ptr<Buffer> buffer_from_resource(
        wl_resource* buffer,
        std::shared_ptr<Executor> const& executor,
        std::vector<std::unique_ptr<wl_resource, void(*)(wl_resource*)>>&& frames) = 0;
};
}
}

#endif //MIR_PLATFORM_GRAPHICS_WAYLAND_ALLOCATOR_H_
