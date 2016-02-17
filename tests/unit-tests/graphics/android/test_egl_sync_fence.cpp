/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include <mir/test/doubles/mock_egl.h>
#include "egl_sync_fence.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <array>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
using namespace testing;

struct EglSyncFence : ::testing::Test
{
    NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mg::EGLSyncExtensions> sync_extensions{std::make_shared<mg::EGLSyncExtensions>()};
    std::array<int,2> fake_fences;
    void* fake_fence0{&fake_fences[0]};
    void* fake_fence1{&fake_fences[1]};
};

TEST_F(EglSyncFence, creation_failure_throws)
{
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay());
    EXPECT_CALL(mock_egl, eglCreateSyncKHR(mock_egl.fake_egl_display, EGL_SYNC_FENCE_KHR, nullptr))
        .WillOnce(Return(EGL_NO_SYNC_KHR));

    mg::EGLSyncFence fence(sync_extensions);
    EXPECT_THROW({
        fence.raise();
    }, std::runtime_error); 
}

TEST_F(EglSyncFence, raise_sets_sync_point)
{
    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay());
    EXPECT_CALL(mock_egl, eglCreateSyncKHR(mock_egl.fake_egl_display, EGL_SYNC_FENCE_KHR, nullptr))
        .WillOnce(Return(fake_fence0));
    EXPECT_CALL(mock_egl, eglDestroySyncKHR(mock_egl.fake_egl_display, fake_fence0))
        .InSequence(seq); //destructor should call this

    mg::EGLSyncFence fence(sync_extensions);
    fence.raise(); 
}

TEST_F(EglSyncFence, can_wait_for_fence)
{
    std::chrono::nanoseconds ns(1456);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglCreateSyncKHR(mock_egl.fake_egl_display, EGL_SYNC_FENCE_KHR, nullptr))
        .InSequence(seq)
        .WillOnce(Return(fake_fence0));
    EXPECT_CALL(mock_egl, eglClientWaitSyncKHR(mock_egl.fake_egl_display, fake_fence0, 0, ns.count()))
        .InSequence(seq)
        .WillOnce(Return(EGL_CONDITION_SATISFIED_KHR));
    EXPECT_CALL(mock_egl, eglDestroySyncKHR(mock_egl.fake_egl_display, fake_fence0))
        .InSequence(seq);

    mg::EGLSyncFence fence(sync_extensions);
    fence.raise(); 
    EXPECT_TRUE(fence.wait_for(ns)); 
}

TEST_F(EglSyncFence, can_wait_for_fence_with_timeout)
{
    std::chrono::nanoseconds ns(1456);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglCreateSyncKHR(mock_egl.fake_egl_display, EGL_SYNC_FENCE_KHR, nullptr))
        .InSequence(seq)
        .WillOnce(Return(fake_fence0));
    EXPECT_CALL(mock_egl, eglClientWaitSyncKHR(mock_egl.fake_egl_display, fake_fence0, 0, ns.count()))
        .InSequence(seq)
        .WillOnce(Return(EGL_TIMEOUT_EXPIRED_KHR));
    EXPECT_CALL(mock_egl, eglDestroySyncKHR(mock_egl.fake_egl_display, fake_fence0))
        .InSequence(seq);

    mg::EGLSyncFence fence(sync_extensions);
    fence.raise(); 
    EXPECT_FALSE(fence.wait_for(ns)); 
}

TEST_F(EglSyncFence, can_clear_without_waiting)
{
    std::chrono::nanoseconds ns(1456);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglCreateSyncKHR(mock_egl.fake_egl_display, EGL_SYNC_FENCE_KHR, nullptr))
        .InSequence(seq)
        .WillOnce(Return(fake_fence0));
    EXPECT_CALL(mock_egl, eglDestroySyncKHR(mock_egl.fake_egl_display, fake_fence0))
        .InSequence(seq);

    mg::EGLSyncFence fence(sync_extensions);
    fence.raise();
    fence.reset();
    Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(EglSyncFence, repeated_raises_clear_existing_fence)
{
    std::chrono::milliseconds one_ms(1);
    auto reasonable_value = std::chrono::duration_cast<std::chrono::nanoseconds>(one_ms);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglCreateSyncKHR(mock_egl.fake_egl_display, EGL_SYNC_FENCE_KHR, nullptr))
        .InSequence(seq)
        .WillOnce(Return(fake_fence0));
    EXPECT_CALL(mock_egl, eglClientWaitSyncKHR(mock_egl.fake_egl_display, fake_fence0, 0, reasonable_value.count()))
        .InSequence(seq)
        .WillOnce(Return(EGL_CONDITION_SATISFIED_KHR));
    EXPECT_CALL(mock_egl, eglDestroySyncKHR(mock_egl.fake_egl_display, fake_fence0))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglCreateSyncKHR(mock_egl.fake_egl_display, EGL_SYNC_FENCE_KHR, nullptr))
        .InSequence(seq)
        .WillOnce(Return(fake_fence1));
    EXPECT_CALL(mock_egl, eglDestroySyncKHR(mock_egl.fake_egl_display, fake_fence1))
        .InSequence(seq);

    mg::EGLSyncFence fence(sync_extensions);
    fence.raise(); 
    fence.raise(); 
}
