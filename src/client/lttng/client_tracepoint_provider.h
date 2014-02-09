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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_LTTNG_CLIENT_TRACEPOINT_PROVIDER_H_
#define MIR_CLIENT_LTTNG_CLIENT_TRACEPOINT_PROVIDER_H_

#include "mir/report/lttng/tracepoint_provider.h"

namespace mir
{
namespace client
{
namespace lttng
{

class ClientTracepointProvider : public mir::report::lttng::TracepointProvider
{
public:
    ClientTracepointProvider();
};

}
}
}

#endif /* MIR_CLIENT_LTTNG_CLIENT_TRACEPOINT_PROVIDER_H_ */
