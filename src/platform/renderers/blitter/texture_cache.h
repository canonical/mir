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
    explicit TextureCache(std::shared_ptr<graphics::BlitterRenderingProvider> blitter);

    /// Get a texture containing the current contents of `buffer`
    ///
    /// \throws std::runtime_error if the buffer cannot be uploaded
    auto get(graphics::Buffer const& buffer) -> graphics::gl::Texture&;

    /// Drop every texture not returned by `get()` since the last call
    void drop_unused();

    /// Drop every cached texture
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
        void upload(graphics::BlitterRenderingProvider& blitter, graphics::Buffer const& buffer);

    private:
        common::TextureHandle const texture;
    };

    struct Entry
    {
        std::unique_ptr<MappedBufferTexture> texture;
        bool used_this_frame;
    };

    std::shared_ptr<graphics::BlitterRenderingProvider> const blitter;
    std::map<graphics::BufferID, Entry> textures;
};
}

#endif
