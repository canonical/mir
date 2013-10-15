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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_SESSION_CREATOR_H_
#define MIR_FRONTEND_SESSION_CREATOR_H_

#include <boost/asio.hpp>

#include <memory>

namespace mir
{
namespace frontend
{
class SessionCreator
{
public:
    virtual void create_session_for(std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket) = 0;

protected:
    SessionCreator() = default;
    virtual ~SessionCreator() noexcept = default;
    SessionCreator(SessionCreator const&) = delete;
    SessionCreator& operator=(SessionCreator const&) = delete;
};
}
}

#endif /* MIR_FRONTEND_SESSION_CREATOR_H_ */
