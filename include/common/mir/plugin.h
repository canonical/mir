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

namespace mir
{

/**
 * Plugin is a base class designed to underpin any class whose implementation
 * exists in a dynamically run-time loaded library.
 * Plugin provides a foolproof check that you don't accidentally unload the
 * library whose code you're still executing.
 */
class Plugin
{
public:
    // Must never be inlined!
    Plugin();
    virtual ~Plugin() noexcept;

    void keep_library_loaded(std::shared_ptr<void> const&);
    std::shared_ptr<void> keep_library_loaded() const;
private:
    std::shared_ptr<void> library;
};

} // namespace mir

#endif // MIR_PLUGIN_H_
