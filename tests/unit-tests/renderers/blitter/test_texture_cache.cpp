/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "src/platform/renderers/blitter/texture_cache.h"

#include <mir/test/doubles/mock_blitter_rendering_provider.h>
#include <mir/test/doubles/mock_gl.h>
#include <mir/test/doubles/stub_buffer.h>

#include <GLES2/gl2ext.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mrb = mir::renderer::blitter;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
/// The image specified for a texture by a glTexImage2D call
struct Upload
{
    GLuint texture;
    GLint internal_format;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    void const* pixels;
    GLint row_length;
};

auto uploaded_from(GLuint texture, mtd::StubBuffer const& buffer)
{
    return AllOf(
        Field(&Upload::texture, Eq(texture)),
        Field(&Upload::width, Eq(buffer.size().width.as_int())),
        Field(&Upload::height, Eq(buffer.size().height.as_int())),
        Field(&Upload::pixels, Eq(buffer.written_pixels.data())));
}

/// A glGenTextures action that allocates sequential non-zero names
auto generate_names()
{
    return [next = GLuint{1}](GLsizei n, GLuint* names) mutable
    {
        for (auto i = 0; i < n; ++i)
        {
            names[i] = next++;
        }
    };
}

/// A glGenTextures action that allocates sequential non-zero names, recording them in \p names
auto generate_into(std::vector<GLuint>& names)
{
    return [&names](GLsizei n, GLuint* out)
    {
        for (auto i = 0; i < n; ++i)
        {
            out[i] = static_cast<GLuint>(names.size() + 1);
            names.push_back(out[i]);
        }
    };
}

/// A glDeleteTextures action that records the deleted names in \p names
auto delete_into(std::vector<GLuint>& names)
{
    return [&names](GLsizei n, GLuint const* deleted)
    {
        names.insert(names.end(), deleted, deleted + n);
    };
}

class TextureCacheTest : public Test
{
public:
    TextureCacheTest()
    {
        ON_CALL(mock_gl, glGenTextures(_, _)).WillByDefault(generate_names());
        ON_CALL(*blitter, map_buffer(_)).WillByDefault(
            [](mg::Buffer const& buffer) { return dynamic_cast<mtd::StubBuffer const&>(buffer).map_readable(); });
    }

    /// Record every texture image specified into \p uploads, along with the texture it was specified for
    void record_uploads_into(std::vector<Upload>& uploads)
    {
        auto const bound_texture = std::make_shared<GLuint>(0);
        auto const unpack_row_length = std::make_shared<GLint>(0);

        ON_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, _)).WillByDefault(
            [bound_texture](GLenum, GLuint name) { *bound_texture = name; });
        ON_CALL(mock_gl, glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, _)).WillByDefault(
            [unpack_row_length](GLenum, GLint value) { *unpack_row_length = value; });
        ON_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, _, _, _, _, _, _, _)).WillByDefault(
            [&uploads, bound_texture, unpack_row_length](
                GLenum, GLint, GLint internal_format, GLsizei width, GLsizei height, GLint,
                GLenum format, GLenum type, GLvoid const* pixels)
            {
                uploads.push_back(
                    {*bound_texture, internal_format, width, height, format, type, pixels, *unpack_row_length});
            });
    }

    NiceMock<mtd::MockGL> mock_gl;
    std::shared_ptr<NiceMock<mtd::MockBlitterRenderingProvider>> const blitter{
        std::make_shared<NiceMock<mtd::MockBlitterRenderingProvider>>()};
};
}

TEST_F(TextureCacheTest, get_maps_buffer_through_blitter)
{
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    EXPECT_CALL(*blitter, map_buffer(Ref(buffer)));

    cache.get(buffer);
}

TEST_F(TextureCacheTest, get_uploads_buffer_contents_into_returned_texture)
{
    std::vector<Upload> uploads;
    record_uploads_into(uploads);
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    auto const texture = cache.get(buffer).tex_id();

    EXPECT_THAT(texture, Ne(0u));
    EXPECT_THAT(uploads, ElementsAre(uploaded_from(texture, buffer)));
}

TEST_F(TextureCacheTest, get_uploads_buffer_in_matching_gl_format)
{
    mtd::StubBuffer const buffer{geom::Size{4, 3}, mir_pixel_format_abgr_8888};
    mrb::TextureCache cache{blitter};

    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _, _, _, GL_RGBA, GL_UNSIGNED_BYTE, _));

    cache.get(buffer);
}

TEST_F(TextureCacheTest, get_uploads_buffer_with_padded_stride)
{
    std::vector<Upload> uploads;
    record_uploads_into(uploads);
    auto const format = mir_pixel_format_abgr_8888;
    auto const padded_row_px = 8;
    mtd::StubBuffer const buffer{geom::Size{4, 3}, format, geom::Stride{padded_row_px * MIR_BYTES_PER_PIXEL(format)}};
    mrb::TextureCache cache{blitter};

    cache.get(buffer);

    ASSERT_THAT(uploads, SizeIs(1));
    EXPECT_THAT(uploads[0].width, Eq(4));
    EXPECT_THAT(uploads[0].row_length, Eq(padded_row_px));
}

TEST_F(TextureCacheTest, get_returns_texture_with_gl_layout)
{
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    EXPECT_THAT(cache.get(buffer).layout(), Eq(mg::gl::Texture::Layout::GL));
}

TEST_F(TextureCacheTest, get_returns_same_texture_for_same_buffer)
{
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    EXPECT_CALL(mock_gl, glGenTextures(_, _));

    auto& first = cache.get(buffer);
    auto& second = cache.get(buffer);

    EXPECT_THAT(&second, Eq(&first));
}

TEST_F(TextureCacheTest, get_returns_distinct_textures_for_distinct_buffers)
{
    mtd::StubBuffer const buffer_a{geom::Size{4, 3}};
    mtd::StubBuffer const buffer_b{geom::Size{2, 5}};
    mrb::TextureCache cache{blitter};

    auto& texture_a = cache.get(buffer_a);
    auto& texture_b = cache.get(buffer_b);

    EXPECT_THAT(&texture_b, Ne(&texture_a));
    EXPECT_THAT(texture_b.tex_id(), Ne(texture_a.tex_id()));
}

TEST_F(TextureCacheTest, get_reuploads_buffer_contents_on_every_call)
{
    std::vector<Upload> uploads;
    record_uploads_into(uploads);
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    auto const texture = cache.get(buffer).tex_id();
    cache.get(buffer);

    EXPECT_THAT(uploads, ElementsAre(uploaded_from(texture, buffer), uploaded_from(texture, buffer)));
}

TEST_F(TextureCacheTest, get_throws_when_buffer_cannot_be_mapped)
{
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};
    ON_CALL(*blitter, map_buffer(_)).WillByDefault(ReturnNull());

    EXPECT_THROW(cache.get(buffer), std::runtime_error);
}

TEST_F(TextureCacheTest, get_throws_when_buffer_format_has_no_gl_equivalent)
{
    mtd::StubBuffer const buffer{geom::Size{4, 3}, mir_pixel_format_bgr_888};
    mrb::TextureCache cache{blitter};

    EXPECT_THROW(cache.get(buffer), std::runtime_error);
}

TEST_F(TextureCacheTest, drop_unused_keeps_textures_used_since_last_call)
{
    std::vector<GLuint> generated;
    std::vector<GLuint> deleted;
    ON_CALL(mock_gl, glGenTextures(_, _)).WillByDefault(generate_into(generated));
    ON_CALL(mock_gl, glDeleteTextures(_, _)).WillByDefault(delete_into(deleted));
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    auto const texture = cache.get(buffer).tex_id();
    cache.drop_unused();

    EXPECT_THAT(deleted, IsEmpty());
    EXPECT_THAT(cache.get(buffer).tex_id(), Eq(texture));
    EXPECT_THAT(generated, SizeIs(1));
}

TEST_F(TextureCacheTest, drop_unused_deletes_textures_not_used_since_last_call)
{
    std::vector<GLuint> deleted;
    ON_CALL(mock_gl, glDeleteTextures(_, _)).WillByDefault(delete_into(deleted));
    mtd::StubBuffer const buffer_a{geom::Size{4, 3}};
    mtd::StubBuffer const buffer_b{geom::Size{2, 5}};
    mrb::TextureCache cache{blitter};

    auto const texture_a = cache.get(buffer_a).tex_id();
    auto const texture_b = cache.get(buffer_b).tex_id();
    cache.drop_unused();
    cache.get(buffer_a);
    cache.drop_unused();

    EXPECT_THAT(deleted, ElementsAre(texture_b));
    EXPECT_THAT(cache.get(buffer_a).tex_id(), Eq(texture_a));
}

TEST_F(TextureCacheTest, texture_unused_between_two_drops_is_deleted)
{
    std::vector<GLuint> deleted;
    ON_CALL(mock_gl, glDeleteTextures(_, _)).WillByDefault(delete_into(deleted));
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    auto const texture = cache.get(buffer).tex_id();
    cache.drop_unused();
    cache.drop_unused();

    EXPECT_THAT(deleted, ElementsAre(texture));
}

TEST_F(TextureCacheTest, get_after_texture_is_dropped_creates_new_texture)
{
    std::vector<GLuint> generated;
    std::vector<Upload> uploads;
    ON_CALL(mock_gl, glGenTextures(_, _)).WillByDefault(generate_into(generated));
    record_uploads_into(uploads);
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    cache.get(buffer);
    cache.drop_unused();
    cache.drop_unused();
    auto const texture = cache.get(buffer).tex_id();

    EXPECT_THAT(generated, SizeIs(2));
    EXPECT_THAT(texture, Eq(generated.back()));
    EXPECT_THAT(uploads.back(), uploaded_from(texture, buffer));
}

TEST_F(TextureCacheTest, drop_unused_on_empty_cache_deletes_nothing)
{
    mrb::TextureCache cache{blitter};

    EXPECT_CALL(mock_gl, glDeleteTextures(_, _)).Times(0);

    cache.drop_unused();
}

TEST_F(TextureCacheTest, invalidate_deletes_all_textures)
{
    std::vector<GLuint> deleted;
    ON_CALL(mock_gl, glDeleteTextures(_, _)).WillByDefault(delete_into(deleted));
    mtd::StubBuffer const buffer_a{geom::Size{4, 3}};
    mtd::StubBuffer const buffer_b{geom::Size{2, 5}};
    mrb::TextureCache cache{blitter};

    auto const texture_a = cache.get(buffer_a).tex_id();
    auto const texture_b = cache.get(buffer_b).tex_id();
    cache.invalidate();

    EXPECT_THAT(deleted, UnorderedElementsAre(texture_a, texture_b));
}

TEST_F(TextureCacheTest, get_after_invalidate_creates_new_texture)
{
    std::vector<GLuint> generated;
    std::vector<Upload> uploads;
    ON_CALL(mock_gl, glGenTextures(_, _)).WillByDefault(generate_into(generated));
    record_uploads_into(uploads);
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    cache.get(buffer);
    cache.invalidate();
    auto const texture = cache.get(buffer).tex_id();

    EXPECT_THAT(generated, SizeIs(2));
    EXPECT_THAT(texture, Eq(generated.back()));
    EXPECT_THAT(uploads.back(), uploaded_from(texture, buffer));
}

TEST_F(TextureCacheTest, drop_unused_after_invalidate_deletes_nothing_further)
{
    std::vector<GLuint> deleted;
    ON_CALL(mock_gl, glDeleteTextures(_, _)).WillByDefault(delete_into(deleted));
    mtd::StubBuffer const buffer{geom::Size{4, 3}};
    mrb::TextureCache cache{blitter};

    auto const texture = cache.get(buffer).tex_id();
    cache.invalidate();
    cache.drop_unused();
    cache.drop_unused();

    EXPECT_THAT(deleted, ElementsAre(texture));
}

TEST_F(TextureCacheTest, destroying_cache_deletes_its_textures)
{
    std::vector<GLuint> deleted;
    ON_CALL(mock_gl, glDeleteTextures(_, _)).WillByDefault(delete_into(deleted));
    mtd::StubBuffer const buffer_a{geom::Size{4, 3}};
    mtd::StubBuffer const buffer_b{geom::Size{2, 5}};
    auto cache = std::make_unique<mrb::TextureCache>(blitter);

    auto const texture_a = cache->get(buffer_a).tex_id();
    auto const texture_b = cache->get(buffer_b).tex_id();
    cache.reset();

    EXPECT_THAT(deleted, UnorderedElementsAre(texture_a, texture_b));
}
