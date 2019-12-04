/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_DECORATION_NULL_MANAGER_H_
#define MIR_SHELL_DECORATION_NULL_MANAGER_H_

#include "manager.h"

namespace mir
{
namespace shell
{
namespace decoration
{
class NullManager
    : public Manager
{
public:
    void init(std::weak_ptr<shell::Shell> const&) override {}
    void decorate(std::shared_ptr<scene::Surface> const&) override {}
    void undecorate(std::shared_ptr<scene::Surface> const&) override {}
    void undecorate_all() override {}
};
}
}
}

#endif /* MIR_SHELL_DECORATION_NULL_MANAGER_H_ */
