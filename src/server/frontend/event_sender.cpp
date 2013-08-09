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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/frontend/client_constants.h"
#include "mir/graphics/display_configuration.h"
#include "event_sender.h"
#include "message_sender.h"
#include "protobuf_buffer_packer.h"

#include "mir_protobuf_wire.pb.h"
#include "mir_protobuf.pb.h"

namespace mg = mir::graphics;
namespace mfd = mir::frontend::detail;
namespace mp = mir::protobuf;

mfd::EventSender::EventSender(std::shared_ptr<MessageSender> const& socket_sender)
 : sender(socket_sender)
{
}

void mfd::EventSender::handle_event(MirEvent const& e)
{
    // Limit the types of events we wish to send over protobuf, for now.
    if (e.type == mir_event_type_surface)
    {
        // In future we might send multiple events, or insert them into messages
        // containing other responses, but for now we send them individually.
        mp::EventSequence seq;
        mp::Event *ev = seq.add_event();
        ev->set_raw(&e, sizeof(MirEvent));

        send_event_sequence(seq);
    }
}

void mfd::EventSender::handle_display_config_change(graphics::DisplayConfiguration const& display_config)
{
    mp::EventSequence seq;
    auto message = seq.mutable_display_configuration();

    display_config.for_each_output([&message](mg::DisplayConfigurationOutput const& config)
    {
        auto disp = message->add_display_output();
        mfd::pack_protobuf_display_output(disp, config); 
    });

    send_event_sequence(seq);
}

void mfd::EventSender::send_event_sequence(mp::EventSequence& seq)
{
    std::string send_buffer;
    send_buffer.reserve(serialization_buffer_size);
    seq.SerializeToString(&send_buffer);

    mir::protobuf::wire::Result result;
    result.add_events(send_buffer);
    result.SerializeToString(&send_buffer);
    sender->send(send_buffer);
}
