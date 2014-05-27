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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "mir_toolkit/mir_trust_session.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;

namespace
{
struct TrustSessionClientAPI : mtf::BasicClientServerFixture<mtf::StubbedServerConfiguration>
{
    static constexpr int arbitrary_base_session_id = __LINE__;
    static constexpr mir_trust_session_event_callback null_event_callback = nullptr;

    MirTrustSession* trust_session = nullptr;
    int started = 0;
    int stopped = 0;
};

void trust_session_start_callback(MirTrustSession* session, void* context)
{
    TrustSessionClientAPI* self = static_cast<TrustSessionClientAPI*>(context);
    self->trust_session = session;
    self->started++;
}

void trust_session_stop_callback(MirTrustSession* /* session */, void* context)
{
    TrustSessionClientAPI* self = static_cast<TrustSessionClientAPI*>(context);
    self->stopped++;
}
}

using namespace testing;

TEST_F(TrustSessionClientAPI, can_start_and_stop_a_trust_session)
{
    mir_wait_for(mir_connection_start_trust_session(connection, arbitrary_base_session_id, trust_session_start_callback, null_event_callback, this));
    ASSERT_THAT(trust_session, Ne(nullptr));
    EXPECT_THAT(started, Eq(1));

    mir_wait_for(mir_trust_session_stop(trust_session, trust_session_stop_callback, this));
    EXPECT_THAT(stopped, Eq(1));

    mir_trust_session_release(trust_session);
}

// TODO there should be testing with a non-null mir_trust_session_event_callback
