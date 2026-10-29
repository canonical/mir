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

#ifndef MIR_RENDERER_TEXTURE_CACHE_H_
#define MIR_RENDERER_TEXTURE_CACHE_H_


#include "common/gl_handles.h"

#include <mir/graphics/buffer_id.h>
#include <mir/graphics/texture.h>

#include <map>
#include <memory>

namespace mir::graphics
{
class Buffer;
class BlitterRenderingProvider;
}

namespace mir::renderer::blitter
{
class SoftwareEGLContext;

/// Per-renderer cache of textures.
///
/// The textures themselves are expensive to create, and the GL fallback is by
/// definition on the slow path, so we keep them around for as long as their
/// buffer keeps being submitted.
///
/// Contents are re-uploaded on each use because the blitter provider gives us
/// no way to know whether a mapping has changed.
class TextureCache
{
public:
    TextureCache(
        std::shared_ptr<SoftwareEGLContext> context,
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter);

    /// \note Leaves the caller's current EGL state unchanged
    ~TextureCache();

    TextureCache(TextureCache const&) = delete;
    auto operator=(TextureCache const&) -> TextureCache& = delete;

    /// Make the cache's context current and get a texture containing the current contents of `buffer`
    ///
    /// The cache's context is left current, ready to draw with the returned texture. The texture
    /// remains valid until it is dropped by `drop_unused()` or `invalidate()`, or the cache is destroyed.
    ///
    /// \throws std::runtime_error if the context cannot be made current or the buffer cannot be uploaded
    auto get(graphics::Buffer const& buffer) -> graphics::gl::Texture&;

    /// Drop every texture not returned by `get()` since the last call
    ///
    /// \note Leaves the caller's current EGL state unchanged
    /// \throws std::runtime_error if the context cannot be made current, no textures are dropped
    void drop_unused();

    /// Drop every cached texture
    ///
    /// \note Leaves the caller's current EGL state unchanged
    /// \throws std::runtime_error if the context cannot be made current, no textures are dropped
    void invalidate();

private:
    class MappedBufferTexture : public graphics::gl::Texture
    {
    public:
        MappedBufferTexture();

        auto shader(graphics::gl::ProgramFactory& factory) const -> graphics::gl::Program const& override;
        auto layout() const -> Layout override;
        void bind() override;
        auto tex_id() const -> GLuint override;
        void add_syncpoint() override;
        void upload(graphics::Buffer const& buffer);

    private:
        common::TextureHandle const texture;
    };

    struct Entry
    {
        std::unique_ptr<MappedBufferTexture> texture;
        bool used_this_frame = false;
    };

    std::shared_ptr<SoftwareEGLContext> context;
    std::shared_ptr<graphics::BlitterRenderingProvider> blitter;
    std::map<graphics::BufferID, Entry> textures;
};
}

#endif
