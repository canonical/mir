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

// Polymorphism: classes with virtual methods, an override in a derived
// class, and a pure-virtual (abstract) class.  Each polymorphic class
// should emit vtable/typeinfo and non-virtual-thunk symbols.  A
// pure-virtual method itself must NOT be emitted.

namespace mir
{

struct Base
{
    virtual ~Base();
    virtual void poly();
    void non_virtual();
};

struct Derived : Base
{
    void poly() override;
};

class Abstract
{
public:
    virtual void pure() = 0;
    virtual void concrete();
};

}
