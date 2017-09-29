/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_CLIENT_RPC_MIR_PROTOBUF_RPC_CHANNEL_H_
#define MIR_CLIENT_RPC_MIR_PROTOBUF_RPC_CHANNEL_H_

#include "mir_basic_rpc_channel.h"
#include "stream_transport.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"

#include "../lifecycle_control.h"
#include "../ping_handler.h"
#include "../error_handler.h"

#include <thread>
#include <atomic>
#include <experimental/optional>

namespace mir
{

namespace input
{
class InputDevices;

namespace receiver
{
class InputReceiverReport;
}

}
namespace client
{
class DisplayConfiguration;
class SurfaceMap;
class EventSink;
class AsyncBufferFactory;

namespace rpc
{

class RpcReport;

class MirProtobufRpcChannel :
        public MirBasicRpcChannel,
        public StreamTransport::Observer,
        public dispatch::Dispatchable
{
public:
    MirProtobufRpcChannel(std::unique_ptr<StreamTransport> transport,
                          std::shared_ptr<SurfaceMap> const& surface_map,
                          std::shared_ptr<AsyncBufferFactory> const& buffer_factory,
                          std::shared_ptr<DisplayConfiguration> const& disp_config,
                          std::shared_ptr<input::InputDevices> const& input_devices,
                          std::shared_ptr<RpcReport> const& rpc_report,
                          std::shared_ptr<input::receiver::InputReceiverReport> const& input_report,
                          std::shared_ptr<LifecycleControl> const& lifecycle_control,
                          std::shared_ptr<PingHandler> const& ping_handler,
                          std::shared_ptr<ErrorHandler> const& error_handler,
                          std::shared_ptr<EventSink> const& event_sink);

    ~MirProtobufRpcChannel() = default;

    // StreamTransport::Observer
    void on_data_available() override;
    void on_disconnected() override;

    // Dispatchable
    Fd watch_fd() const override;
    bool dispatch(mir::dispatch::FdEvents events) override;
    mir::dispatch::FdEvents relevant_events() const override;

    /**
     * \brief Switch the RpcChannel into out-of-order mode
     *
     * The first CallMethod after this method is called will be processed
     * out of order - no server responses will be processed until the response
     * for the next CallMethod is processed.
     *
     * After the response for the next CallMethod is processed, normal processing
     * is resumed.
     *
     * No messages are discarded, only delayed.
     */
    void process_next_request_first();

    void call_method(
        std::string const& method_name,
        google::protobuf::MessageLite const* parameters,
        google::protobuf::MessageLite* response,
        google::protobuf::Closure* complete) override;
    void discard_future_calls() override;
    void wait_for_outstanding_calls() override;

private:
    std::shared_ptr<RpcReport> const rpc_report;
    detail::PendingCallCache pending_calls;

    std::shared_ptr<input::receiver::InputReceiverReport> const input_report;

    std::mutex discard_mutex;
    bool discard{false};

    static constexpr size_t size_of_header = 2;
    detail::SendBuffer header_bytes;
    detail::SendBuffer body_bytes;

    void receive_file_descriptors(google::protobuf::MessageLite* response);
    template<class MessageType>
    void receive_any_file_descriptors_for(MessageType* response);
    void send_message(mir::protobuf::wire::Invocation const& body,
                      mir::protobuf::wire::Invocation const& invocation,
                      std::vector<mir::Fd>& fds);

    void read_message();
    void process_event_sequence(std::string const& event);

    void notify_disconnected();

    std::weak_ptr<SurfaceMap> surface_map;
    std::shared_ptr<AsyncBufferFactory> const buffer_factory;
    std::shared_ptr<DisplayConfiguration> display_configuration;
    std::shared_ptr<input::InputDevices> input_devices;
    std::shared_ptr<LifecycleControl> lifecycle_control;
    std::shared_ptr<PingHandler> const ping_handler;
    std::shared_ptr<ErrorHandler> const error_handler;
    std::shared_ptr<EventSink> event_sink;
    std::atomic<bool> disconnected;
    std::mutex read_mutex;
    std::mutex write_mutex;

    bool prioritise_next_request{false};
    std::experimental::optional<uint32_t> id_to_wait_for;

    /* We use the guarantee that the transport's destructor blocks until
     * pending processing has finished to ensure that on_data_available()
     * isn't called after the members it relies on are destroyed.
     *
     * This means that anything that owns a reference to the transport
     * needs to be after anything that can be accessed from on_data_available().
     *
     * For simplicity's sake keep all of the dispatch infrastructure at the
     * end to guarantee this.
     */
    std::shared_ptr<StreamTransport> const transport;
    std::shared_ptr<mir::dispatch::ActionQueue> const delayed_processor;
    mir::dispatch::MultiplexingDispatchable multiplexer;
};

}
}
}

#endif /* MIR_CLIENT_RPC_MIR_PROTOBUF_RPC_CHANNEL_H_ */
