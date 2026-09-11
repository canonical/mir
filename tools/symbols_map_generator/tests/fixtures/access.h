/*
 * Copyright © Canonical Ltd.
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
 */

// Access control.  Private members are never emitted.  Protected members
// ARE emitted (they are part of the ABI: derived classes can call them),
// even though the traverse_ast comment only mentions "private".  This
// fixture pins that behaviour so it is not changed by accident.

namespace mir
{

class AccessLevels
{
public:
    void public_method();

protected:
    void protected_method();

private:
    void private_method();
    int private_field;
};

}
