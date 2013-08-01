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

#include "src/server/frontend/protobuf_buffer_packer.h"

#include "mir_protobuf.pb.h"
#include <gtest/gtest.h>

namespace mfd=mir::frontend::detail;
namespace mp=mir::protobuf;
namespace geom=mir::geometry;

TEST(TestEventSender, display_send)
{
    StubConfiguration config;

    auto msg_validator = [](std::string){
    //cheggit
    };

    EXPECT_CALL(mock_msg_sender, send(_))
        .Times(1)
        .WillOnce(Invoke(msg_validator));
    EventSender sender(mock_msg_sender);

    sender.send_display_config(config);
}
