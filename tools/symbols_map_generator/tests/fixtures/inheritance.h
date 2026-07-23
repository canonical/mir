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

// Inheritance-driven vtable detection.  `Inheritor` declares no virtual
// methods of its own but inherits a vtable from `PolyBase`.  Correct
// behaviour is that it should still emit a vtable symbol.  Virtual
// inheritance (VB1/VB2/Diamond) additionally requires VTT symbols.

namespace mir
{

struct PolyBase
{
    virtual ~PolyBase();
    virtual void poly();
};

struct Inheritor : PolyBase
{
    void non_virtual_helper();
};

struct VB1 : virtual PolyBase
{
};

struct VB2 : virtual PolyBase
{
};

struct Diamond : VB1, VB2
{
};

}
