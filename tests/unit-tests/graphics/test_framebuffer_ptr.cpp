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

#include <mir/graphics/framebuffer_ptr.h>
#include <mir/graphics/platform.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <utility>

namespace geom = mir::geometry;
namespace mg = mir::graphics;

using namespace testing;

namespace
{
class FakeFramebuffer : public mg::Framebuffer
{
public:
    explicit FakeFramebuffer(bool& destroyed) : destroyed{destroyed} {}
    ~FakeFramebuffer() override { destroyed = true; }

    auto size() const -> geom::Size override { return {}; }

private:
    bool& destroyed;
};

class OtherFramebuffer : public mg::Framebuffer
{
public:
    auto size() const -> geom::Size override { return {}; }
};

/// A release function recording how often it's been called
auto count_into(int& releases)
{
    return mg::FramebufferDeleter{[&releases] { ++releases; }};
}
}

TEST(FramebufferPtr, deletes_framebuffer_by_default)
{
    bool destroyed{false};
    mg::FramebufferPtr fb{new FakeFramebuffer{destroyed}};

    fb.reset();

    EXPECT_TRUE(destroyed);
}

TEST(FramebufferPtr, converts_from_plain_unique_ptr_and_deletes_framebuffer)
{
    bool destroyed{false};
    mg::FramebufferPtr fb{std::make_unique<FakeFramebuffer>(destroyed)};

    fb.reset();

    EXPECT_TRUE(destroyed);
}

TEST(FramebufferPtr, calls_release_instead_of_deleting_framebuffer)
{
    bool destroyed{false};
    FakeFramebuffer owned{destroyed};
    int releases{0};
    mg::FramebufferPtr fb{&owned, count_into(releases)};

    fb.reset();

    EXPECT_THAT(releases, Eq(1));
    EXPECT_FALSE(destroyed);
}

TEST(FramebufferPtr, release_is_kept_when_converted_to_shared_ptr)
{
    bool destroyed{false};
    FakeFramebuffer owned{destroyed};
    int releases{0};
    std::shared_ptr<mg::Framebuffer> fb{mg::FramebufferPtr{&owned, count_into(releases)}};

    fb.reset();

    EXPECT_THAT(releases, Eq(1));
    EXPECT_FALSE(destroyed);
}

TEST(FramebufferPtr, release_follows_moved_deleter)
{
    bool destroyed{false};
    FakeFramebuffer owned{destroyed};
    int releases{0};
    mg::FramebufferPtr from{&owned, count_into(releases)};
    mg::FramebufferPtr to;

    to = std::move(from);
    from.reset();
    EXPECT_THAT(releases, Eq(0));

    to.reset();
    EXPECT_THAT(releases, Eq(1));
    EXPECT_FALSE(destroyed);
}

TEST(FramebufferPtr, release_is_called_at_most_once)
{
    bool first_destroyed{false};
    FakeFramebuffer first{first_destroyed};
    bool second_destroyed{false};
    int releases{0};
    mg::FramebufferPtr fb{&first, count_into(releases)};

    fb.reset(new FakeFramebuffer{second_destroyed});
    fb.reset();

    EXPECT_THAT(releases, Eq(1));
    EXPECT_FALSE(first_destroyed);
    EXPECT_TRUE(second_destroyed);
}

TEST(FramebufferCast, keeps_release_on_success)
{
    bool destroyed{false};
    FakeFramebuffer owned{destroyed};
    int releases{0};

    auto fb = mg::framebuffer_cast<FakeFramebuffer>(mg::FramebufferPtr{&owned, count_into(releases)});
    ASSERT_THAT(fb.get(), Eq(&owned));
    EXPECT_THAT(releases, Eq(0));

    fb.reset();
    EXPECT_THAT(releases, Eq(1));
    EXPECT_FALSE(destroyed);
}

TEST(FramebufferCast, disposes_of_framebuffer_on_mismatch)
{
    OtherFramebuffer owned;
    int releases{0};

    auto const fb = mg::framebuffer_cast<FakeFramebuffer>(mg::FramebufferPtr{&owned, count_into(releases)});

    EXPECT_THAT(fb, IsNull());
    EXPECT_THAT(releases, Eq(1));
}
