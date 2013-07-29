/*
 * Copyright © 2013 Canonical Ltd.
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

#include <gtest/gtest.h>

namespace mfd = mir::frontend::detail;

struct SocketSessionTest : public ::testing::Test
{

};

TEST_F(SocketSessionTest, basic_msg)
{
    mfd::SocketSession session(mock_receiver, 0, null_sessions, mock_procesrsor);

    EXPECT_CALL(mock_receiver, async_read_msg(_));
        .Times(1)
        .WillOnce(SaveArg<0>(&fn));
    session.read_next_message();

    testing::Mock::VerifyAndClearExpectations(mock_receiver);

    EXPECT_CALL(mock_receiver, async_read_msg(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&fn2));

    fn(); 
    
    testing::Mock::VerifyAndClearExpectations(mock_receiver);

    EXPECT_CALL(mock_processor, process_message(_))
        .Times(1);
    fn2();
}
