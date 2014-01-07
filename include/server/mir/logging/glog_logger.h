/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/logging/logger.h"

namespace mir
{
namespace logging
{
class GlogLogger : public Logger
{
public:
    GlogLogger(
        char const* argv0,
        int stderrthreshold,
        int minloglevel,
        std::string const& log_dir);

private:
    virtual void log(
        Severity severity,
        const std::string& message,
        const std::string& component) override;
};
}
}
