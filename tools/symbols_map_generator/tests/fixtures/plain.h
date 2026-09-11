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

// A non-polymorphic class: no virtual methods, so no vtable should be
// emitted, but typeinfo is always emitted for class/struct definitions.
// Private members must never be emitted.

namespace mir
{

class Plain
{
public:
    Plain();
    ~Plain();
    void do_thing();
    int value() const;

private:
    void hidden();
    int secret;
};

void free_function(int a);

}
