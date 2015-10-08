/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 * Originally by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/gl/recently_used_cache.h"
#include "mir/test/doubles/mock_gl_buffer.h"
#include "mir/test/doubles/mock_renderable.h"
#include "mir/test/doubles/mock_gl.h"
#include <gtest/gtest.h>

namespace mtd=mir::test::doubles;
namespace mgl=mir::gl;
namespace mg=mir::graphics;

namespace
{

class RecentlyUsedCache : public testing::Test
{
public:
    RecentlyUsedCache()
    {
        using namespace testing;
        mock_buffer = std::make_shared<NiceMock<mtd::MockGLBuffer>>();
        renderable = std::make_shared<NiceMock<mtd::MockRenderable>>();
        ON_CALL(*renderable, buffer())
            .WillByDefault(Return(mock_buffer));
        ON_CALL(*mock_buffer, id())
            .WillByDefault(Return(mg::BufferID(123)));
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    std::shared_ptr<mtd::MockGLBuffer> mock_buffer;
    std::shared_ptr<testing::NiceMock<mtd::MockRenderable>> renderable;
    GLuint const stub_texture{1};
};
}

TEST_F(RecentlyUsedCache, caches_and_uploads_texture_only_on_buffer_changes)
{
    using namespace testing;
    InSequence seq;

    // Frame 1: Texture generated and uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(*mock_buffer,gl_bind_to_texture());

    // Frame 2: Texture found in cache and not re-uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(*mock_buffer,gl_bind_to_texture())
        .Times(0);

    // Frame 3: Texture found in cache but refreshed with new buffer
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(*mock_buffer,gl_bind_to_texture());

    // Frame 4: Stale texture reuploaded following bypass
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(*mock_buffer,gl_bind_to_texture());

    // Frame 5: Texture found in cache and not re-uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));

    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)));

    mgl::RecentlyUsedCache cache;
    cache.load(*renderable);
    cache.drop_unused();

    cache.load(*renderable);
    cache.drop_unused();

    cache.load(*renderable);
    cache.drop_unused();

    cache.invalidate();

    cache.load(*renderable);
    cache.drop_unused();

    cache.load(*renderable);
    cache.drop_unused();
}

TEST_F(RecentlyUsedCache, holds_buffers_till_the_end)
{
    auto old_use_count = mock_buffer.use_count();
    mgl::RecentlyUsedCache cache;
    cache.load(*renderable);
    EXPECT_EQ(old_use_count+1, mock_buffer.use_count());
    cache.drop_unused();
    EXPECT_EQ(old_use_count, mock_buffer.use_count());
}

//LP: #1362444
TEST_F(RecentlyUsedCache, invalidated_buffers_are_reloaded)
{
    ON_CALL(*mock_buffer, id())
        .WillByDefault(testing::Return(mg::BufferID(0)));
    EXPECT_CALL(*mock_buffer,gl_bind_to_texture())
        .Times(2);

    mgl::RecentlyUsedCache cache;
    cache.load(*renderable);
    cache.load(*renderable);
    cache.invalidate();
    cache.load(*renderable);
}
