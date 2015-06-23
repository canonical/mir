/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */


#include "src/server/scene/prompt_session_impl.h"

#include "mir/test/doubles/mock_scene_session.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{

struct PromptSession : public testing::Test
{
    std::shared_ptr<mtd::MockSceneSession> const helper{std::make_shared<::testing::NiceMock<mtd::MockSceneSession>>()};
};

}

TEST_F(PromptSession, start_when_stopped)
{
    ms::PromptSessionImpl prompt_session;
    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_stopped);

    EXPECT_CALL(*helper, start_prompt_session()).Times(1);
    prompt_session.start(helper);

    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_started);
}

TEST_F(PromptSession, stop_when_started)
{
    ms::PromptSessionImpl prompt_session;
    prompt_session.start(helper);
    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_started);

    EXPECT_CALL(*helper, stop_prompt_session()).Times(1);
    prompt_session.stop(helper);

    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_stopped);
}

TEST_F(PromptSession, suspend_when_started)
{
    ms::PromptSessionImpl prompt_session;
    prompt_session.start(helper);
    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_started);

    EXPECT_CALL(*helper, suspend_prompt_session()).Times(1);
    prompt_session.suspend(helper);

    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_suspended);
}

TEST_F(PromptSession, suspend_fails_to_stop_helper_when_not_started)
{
    ms::PromptSessionImpl prompt_session;

    EXPECT_CALL(*helper, suspend_prompt_session()).Times(0);
    prompt_session.suspend(helper);

    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_stopped);
}

TEST_F(PromptSession, resume_when_suspended)
{
    ms::PromptSessionImpl prompt_session;
    prompt_session.start(helper);
    prompt_session.suspend(helper);
    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_suspended);

    EXPECT_CALL(*helper, resume_prompt_session()).Times(1);
    prompt_session.resume(helper);

    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_started);
}

TEST_F(PromptSession, resume_fails_to_stop_helper_when_not_started)
{
    ms::PromptSessionImpl prompt_session;

    EXPECT_CALL(*helper, resume_prompt_session()).Times(0);
    prompt_session.resume(helper);

    EXPECT_EQ(prompt_session.state(), mir_prompt_session_state_stopped);
}
