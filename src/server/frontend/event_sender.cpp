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

#include "event_sender.h"
#include "message_sender.h"
#include "protobuf_buffer_packer.h"
#include "mir_protobuf_wire.pb.h"
#include "mir_protobuf.pb.h"

#include "mir/make_protobuf_object.h"
#include "mir/frontend/client_constants.h"
#include "mir/graphics/display_configuration.h"
#include "mir/variable_length_array.h"
#include "mir/events/event_private.h"

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
    if (e.type != mir_event_type_key && e.type != mir_event_type_motion)
    {
        // In future we might send multiple events, or insert them into messages
        // containing other responses, but for now we send them individually.
        auto seq = mir::make_protobuf_object<mp::EventSequence>();
        mp::Event *ev = seq->add_event();
        ev->set_raw(&e, sizeof(MirEvent));

        send_event_sequence(*seq);
    }
}

void mfd::EventSender::handle_display_config_change(
    graphics::DisplayConfiguration const& display_config)
{
    auto seq = mir::make_protobuf_object<mp::EventSequence>();

    auto protobuf_config = seq->mutable_display_configuration();
    mfd::pack_protobuf_display_configuration(*protobuf_config, display_config);

    send_event_sequence(*seq);
}

void mfd::EventSender::handle_lifecycle_event(
    MirLifecycleState state)
{
    auto seq = mir::make_protobuf_object<mp::EventSequence>();

    auto protobuf_life_event = seq->mutable_lifecycle_event();
    protobuf_life_event->set_new_state(state);

    send_event_sequence(*seq);
}

void mfd::EventSender::send_event_sequence(mp::EventSequence& seq)
{
    mir::VariableLengthArray<frontend::serialization_buffer_size>
        send_buffer{static_cast<size_t>(seq.ByteSize())};

    seq.SerializeWithCachedSizesToArray(send_buffer.data());

    auto result = mir::make_protobuf_object<mir::protobuf::wire::Result>();
    result->add_events(send_buffer.data(), send_buffer.size());
    send_buffer.resize(result->ByteSize());
    result->SerializeWithCachedSizesToArray(send_buffer.data());

    try
    {
        sender->send(reinterpret_cast<char*>(send_buffer.data()), send_buffer.size(), {});
    }
    catch (std::exception const& error)
    {
        // TODO: We should report this state.
        (void) error;
    }
}
