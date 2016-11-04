/*
 * Copyright © 2016 Canonical Ltd.
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
 * Author: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef PLAYGROUND_DIAMOND_H_
#define PLAYGROUND_DIAMOND_H_
#include <GLES2/gl2.h>
#include <EGL/egl.h>

typedef struct
{
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint pos;
    GLuint color;
    GLfloat const* vertices;
    GLfloat const* colors;
    int num_vertices;
} Diamond;

Diamond setup_diamond();
void destroy_diamond(Diamond* info);
void render_diamond(Diamond* info, EGLDisplay egldisplay, EGLSurface eglsurface);

#endif /* PLAYGROUND_DIAMOND_H_ */
