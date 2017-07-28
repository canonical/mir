/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_EXTENSIONS_WINDOW_COORDINATE_TRANSLATION_H_
#define MIR_CLIENT_EXTENSIONS_WINDOW_COORDINATE_TRANSLATION_H_
#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Important note about extension:
 * There are many cases where such a mapping does not exist or would be expensive
 * to calculate. Only Mir servers started with the --debug option will ever provide
 * this extension, and even when --debug is enabled servers are free to
 * return nothing.
 */

/**
 * Get the screen coordinates corresponding to a pair of window coordinates
 * \pre                               The window is valid
 * \param   [in] window               The window
 * \param   [in] x, y                 Surface coordinates to map to screen coordinates
 * \param   [out] screen_x, screen_y  The screen coordinates corresponding to x, y.
 *
 *          This call will only be interesting for automated testing, where both the client
 *          and shell state is known and constrained.
 */
typedef void (*MirExtensionWindowTranslateCoordinates)(
    MirWindow* window,
    int x, int y,
    int* screen_x, int* screen_y);

typedef struct MirExtensionWindowCoordinateTranslationV1
{
    MirExtensionWindowTranslateCoordinates window_translate_coordinates;
} MirExtensionWindowCoordinateTranslationV1;

static inline MirExtensionWindowCoordinateTranslationV1 const*
mir_extension_window_coordinate_translation_v1(MirConnection* connection)
{
    return (MirExtensionWindowCoordinateTranslationV1 const*) mir_connection_request_extension(
        connection, "mir_extension_window_coordinate_translation", 1);
}
#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_WINDOW_COORDINATE_TRANSLATION_H_ */
