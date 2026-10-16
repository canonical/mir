/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_FRAMEBUFFER_PTR_H_
#define MIR_GRAPHICS_FRAMEBUFFER_PTR_H_

#include <functional>
#include <memory>
#include <type_traits>

namespace mir::graphics
{
class Framebuffer;

/// Disposes of a [Framebuffer] that its holder may not own outright.
///
/// By default, this deletes the framebuffer. Constructed with a release function,
/// it calls that instead, allowing the framebuffer's real owner to do something
/// with it when it is released.
class FramebufferDeleter
{
public:
    /// Delete the framebuffer
    FramebufferDeleter() noexcept;

    /// Call \p release instead of deleting the framebuffer.
    ///
    /// \p release is called at most once. If this is again (e.g. after a `reset()` to
    /// a new pointer), it deletes like a default-constructed one.
    explicit FramebufferDeleter(std::move_only_function<void()> release) noexcept;

    /// Allow a plain `std::unique_ptr<T>` to convert to a FramebufferPtr
    template<typename T>
        requires std::is_convertible_v<T*, Framebuffer*>
    FramebufferDeleter(std::default_delete<T>) noexcept : FramebufferDeleter{}
    {}

    FramebufferDeleter(FramebufferDeleter&& from) noexcept;
    auto operator=(FramebufferDeleter&& from) noexcept -> FramebufferDeleter&;
    ~FramebufferDeleter();

    void operator()(Framebuffer* framebuffer);

private:
    std::move_only_function<void()> release;
};

/// An owning pointer to a [Framebuffer] with a custom deleter.
using FramebufferPtr = std::unique_ptr<Framebuffer, FramebufferDeleter>;

/// Downcast \p framebuffer to \p To, keeping its deleter
///
/// \returns the downcast pointer, or null if \p framebuffer is not a \p To (in which
///          case \p framebuffer is disposed of)
template<typename To>
auto framebuffer_cast(FramebufferPtr framebuffer) -> std::unique_ptr<To, FramebufferDeleter>
{
    if (auto const to = dynamic_cast<To*>(framebuffer.get()))
    {
        framebuffer.release();
        return {to, std::move(framebuffer.get_deleter())};
    }
    return nullptr;
}
}

#endif // MIR_GRAPHICS_FRAMEBUFFER_PTR_H_
