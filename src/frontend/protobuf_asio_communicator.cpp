/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir/frontend/protobuf_asio_communicator.h"

namespace mf = mir::frontend;
namespace ba = boost::asio;

// TODO: Switch to std::bind for launching the thread.
mf::ProtobufAsioCommunicator::ProtobufAsioCommunicator(std::string const& socket_file)
    : socket_file((std::remove(socket_file.c_str()), socket_file)),
      acceptor(io_service, socket_file),
      next_id(1)
{
    start_accept();
}

void mf::ProtobufAsioCommunicator::start_accept()
{
    auto session = std::make_shared<Session>(io_service, next_id++);

    acceptor.async_accept(
        session->socket,
        boost::bind(
            &ProtobufAsioCommunicator::on_new_connection,
            this,
            session,
            ba::placeholders::error));
}

void mf::ProtobufAsioCommunicator::start()
{
    auto run_io_service = boost::bind(&ba::io_service::run, &io_service);
    io_service_thread = std::move(std::thread(run_io_service));
}

mf::ProtobufAsioCommunicator::~ProtobufAsioCommunicator()
{
    io_service.stop();
    if (io_service_thread.joinable())
    {
        io_service_thread.join();
    }
    std::remove(socket_file.c_str());
}

void mf::ProtobufAsioCommunicator::on_new_connection(std::shared_ptr<Session> const& session,
                                                     const boost::system::error_code& ec)
{
    if (!ec)
    {
        change_state(session, SessionState::connected);
        read_next_message(session);
    }
    start_accept();
}

void mf::ProtobufAsioCommunicator::read_next_message(std::shared_ptr<Session> const& session)
{
    // Read newline delimited messages for now
    ba::async_read_until(
         session->socket,
         session->message, "\n",
         boost::bind(&mf::ProtobufAsioCommunicator::on_new_message,
                     this, session,
                     ba::placeholders::error));
}

void mf::ProtobufAsioCommunicator::on_new_message(std::shared_ptr<Session> const& session,
                                                  const boost::system::error_code& ec)
{
    if (!ec)
    {
        std::istream in(&session->message);
        std::string message;
        in >> message;
        if (message == "disconnect")
        {
            change_state(session, SessionState::disconnected);
        }
    }
    if (session->state == SessionState::connected)
    {
        read_next_message(session);
    }
}

void mf::ProtobufAsioCommunicator::change_state(std::shared_ptr<Session> const& session,
                                                SessionState new_state)
{
    struct Transition { SessionState from, to; };
    static const Transition valid_transitions[] =
        {
            { SessionState::initialised, SessionState::connected },
            { SessionState::connected, SessionState::disconnected },
        };

    Transition const * t = valid_transitions;
    Transition const * const t_end = valid_transitions + sizeof(valid_transitions);

    while (t != t_end && (t->from != session->state || t->to != new_state))
    {
        ++t;
    }
    session->state = t == t_end ? SessionState::error : new_state;
    session_state_signal(session, session->state);
}

mf::ProtobufAsioCommunicator::SessionStateSignal& mf::ProtobufAsioCommunicator::signal_session_state()
{
    return session_state_signal;
}
