/*
 * Copyright © 2017 Canonical Ltd.
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

#ifndef MIRAL_BOTH_VERSIONS_H
#define MIRAL_BOTH_VERSIONS_H

#include <mir/version.h>

#define MIRAL_FAKE_OLD_SYMBOL(old_sym, new_sym)\
    extern "C" __attribute__((alias(#new_sym))) void old_sym();

#define MIRAL_FAKE_NEW_SYMBOL(old_sym, new_sym)\
    extern "C" __attribute__((alias(#old_sym))) void new_sym();

#define MIRAL_BOTH_VERSIONS(old_sym, new_sym)\
MIRAL_FAKE_OLD_SYMBOL(old_sym, new_sym)

#endif //MIRAL_BOTH_VERSIONS_H
