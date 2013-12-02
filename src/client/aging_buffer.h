/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_CLIENT_AGING_BUFFER_H_
#define MIR_CLIENT_AGING_BUFFER_H_

#include "client_buffer.h"

namespace mir
{
namespace client
{

class AgingBuffer : public ClientBuffer
{
public:
    AgingBuffer();

    virtual uint32_t age() const;
    virtual void increment_age();
    virtual void mark_as_submitted();

private:
    uint32_t buffer_age;
};

}
}

#endif /* MIR_CLIENT_AGING_BUFFER_H_ */

