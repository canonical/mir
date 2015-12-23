/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_PLUGIN_H_
#define MIR_PLUGIN_H_

#include <memory>
#include <vector>

namespace mir
{

/**
 * Plugin is a concrete base class designed to underpin any class whose
 * implementation comes from a dynamically run-time loaded library. As the
 * class is concrete and implemented outside of any such plugin libraries,
 * its destructor serves as a final safe location from which to release the
 * final reference to the SharedLibrary (or whatever) in which your
 * implementation class resides.
 */
class Plugin
{
public:
    // Must never be inlined!
    Plugin();
    virtual ~Plugin();
    void hold_resource(std::shared_ptr<void> const& r);
    static void safely_unload(std::shared_ptr<Plugin>& p);
private:
    std::vector<std::shared_ptr<void>> resources;
};

} // namespace mir

#endif // MIR_PLUGIN_H_
