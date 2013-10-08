/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Ancell <robert.ancell@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_PAUSE_RESUME_LISTENER_H_
#define MIR_TEST_DOUBLES_MOCK_PAUSE_RESUME_LISTENER_H_

#include "mir/pause_resume_listener.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockPauseResumeListener : public mir::PauseResumeListener
{
public:
    MOCK_METHOD0(paused, void());
    MOCK_METHOD0(resumed, void());
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_PAUSE_RESUME_LISTENER_H_ */
