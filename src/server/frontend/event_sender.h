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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_FRONTEND_EVENT_SENDER_H_
#define MIR_FRONTEND_EVENT_SENDER_H_

#include "mir/frontend/event_sink.h"
#include "mir/frontend/fd_sets.h"
#include <memory>

namespace mir
{
namespace graphics { class PlatformIpcOperations; }
namespace protobuf
{
class EventSequence;
}
namespace frontend
{
class MessageSender;

namespace detail
{

class EventSender : public  mir::frontend::EventSink
{
public:
    explicit EventSender(
        std::shared_ptr<MessageSender> const& socket_sender,
        std::shared_ptr<graphics::PlatformIpcOperations> const& buffer_packer);
    void handle_event(MirEvent const& e);
    void handle_lifecycle_event(MirLifecycleState state);
    void handle_display_config_change(graphics::DisplayConfiguration const& config);
    void send_ping(int32_t serial);
    void send_buffer(frontend::BufferStreamId id, graphics::Buffer& buffer, graphics::BufferIpcMsgType);

private:
    void send_event_sequence(protobuf::EventSequence&, FdSets const&);

    std::shared_ptr<MessageSender> const sender;
    std::shared_ptr<graphics::PlatformIpcOperations> const buffer_packer;
};

}
}
}
#endif /* MIR_FRONTEND_EVENT_SENDER_H_ */
