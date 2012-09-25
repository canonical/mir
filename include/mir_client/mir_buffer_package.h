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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_MIR_BUFFER_PACKAGE_H_
#define MIR_CLIENT_MIR_BUFFER_PACKAGE_H_

#include <vector>

namespace mir
{
namespace client
{
/* note: kdub: this is the same thing as BufferIPCPackage on the server side. duplicated to 
               maintain divide between client/server headers */
struct MirBufferPackage
{
    std::vector<int> data;
    std::vector<int> fd;
};

}
}
#endif /* MIR_CLIENT_MIR_BUFFER_PACKAGE_H_ */
