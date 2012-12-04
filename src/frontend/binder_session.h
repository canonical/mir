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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_FRONTEND_BINDER_SESSION_H_
#define MIR_FRONTEND_BINDER_SESSION_H_

#include "message_processor.h"

#include <binder/Binder.h>

namespace mir
{
namespace frontend
{
namespace detail
{
class BinderSession : public MessageSender, public android::BBinder
{
public:
    BinderSession();
    ~BinderSession();

    void set_processor(std::shared_ptr<MessageProcessor> const& processor);

private:

    void send(const std::ostringstream& buffer2);
    void send_fds(std::vector<int32_t> const& fd);

    android::status_t onTransact(uint32_t code,
                                 const android::Parcel& request,
                                 android::Parcel* response,
                                 uint32_t flags);

    std::shared_ptr<MessageProcessor> processor;
};
}
}
}


#endif /* MIR_FRONTEND_BINDER_SESSION_H_ */
