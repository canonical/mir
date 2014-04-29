/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_toolkit/mir_cursor.h"
#include "cursor_representation.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

void mir_cursor_destroy(MirCursor *cursor)
{
    assert(cursor);
    
    if (cursor->name)
        free(cursor->name);

    delete cursor;
}

MirCursor* mir_cursor_disabled()
{
    auto c = new MirCursor;
    c->name = nullptr;
    
    return c;
}

MirCursor* mir_cursor_default()
{
    auto c = new MirCursor;
    c->name = strdup("default");
    
    return c;
}

MirCursor* mir_cursor_from_name(char const* name)
{
    auto c = new MirCursor;
    c->name = strdup(name);
    
    return c;
}
