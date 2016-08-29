/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_UNIT_TESTS_LIBRARY_EXAMPLE_H_
#define MIR_UNIT_TESTS_LIBRARY_EXAMPLE_H_

#include "mir/module_deleter.h"

class SomeInterface
{
public:
    SomeInterface() = default;
    virtual ~SomeInterface() = default;
    virtual void can_be_executed() = 0;

    SomeInterface& operator=(SomeInterface const&) = delete;
    SomeInterface(SomeInterface const&) = delete;
};

using ModuleFunction = mir::UniqueModulePtr<SomeInterface> (*)();

#if defined(__clang__)
#pragma clang diagnostic push
// These functions are given "C" linkage to avoid name-mangling, not for C compatibility.
// (We don't want a warning for doing this intentionally.)
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif
extern "C" mir::UniqueModulePtr<SomeInterface> module_create_instance();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
