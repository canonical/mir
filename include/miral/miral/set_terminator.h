/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_SET_TERMINATOR_H
#define MIRAL_SET_TERMINATOR_H

#include <functional>

namespace mir { class Server; }

namespace miral
{
/// Set handler for termination requests.
/// terminator will be called following receipt of SIGTERM or SIGINT.
/// The default terminator stop()s the server, replacements should probably
/// do the same in addition to any additional shutdown logic.
/// \remark Since MirAL 3.0
class SetTerminator
{
public:
    using Terminator = std::function<void(int signal)>;

    explicit SetTerminator(Terminator const& terminator);
    ~SetTerminator();

    void operator()(mir::Server& server) const;

private:
    Terminator terminator;
};
}

#endif //MIRAL_SET_TERMINATOR_H
