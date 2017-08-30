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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_SET_COMMAND_LINE_HANDLER_H
#define MIRAL_SET_COMMAND_LINE_HANDLER_H

#include <functional>

namespace mir { class Server; }

namespace miral
{
/// Set a handler for any command line options Mir/MirAL does not recognise.
/// This will be invoked if any unrecognised options are found during initialisation.
/// Any unrecognised arguments are passed to this function. The pointers remain valid
/// for the duration of the call only.
/// If set_command_line_handler is not called the default action is to exit by
/// throwing mir::AbnormalExit (which will be handled by the exception handler prior to
/// exiting run().
class SetCommandLineHandler
{
public:
    using Handler = std::function<void(int argc, char const* const* argv)>;

    explicit SetCommandLineHandler(Handler const& handler);
    ~SetCommandLineHandler();

    void operator()(mir::Server& server) const;

private:
    Handler handler;
};
}

#endif //MIRAL_SET_COMMAND_LINE_HANDLER_H
