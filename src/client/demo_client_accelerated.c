/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Kevin DuBois   <kevin.dubois@canonical.com>
 */

#include "mir_client/mir_client_library.h"

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

static char const *socket_file = "/data/tmp/mir_socket";
static MirConnection *connection = 0;
static MirSurface *surface = 0;

static void set_connection(MirConnection *new_connection, void * context)
{
    (void)context;
    connection = new_connection;
}

static void surface_create_callback(MirSurface *new_surface, void *context)
{
    (void)context;
    surface = new_surface;
}

static void surface_next_callback(MirSurface *new_surface, void *context)
{
}

static void surface_release_callback(MirSurface *old_surface, void *context)
{
    (void)old_surface;
    (void)context;
    surface = 0;
}


/* animation state variables */
GLfloat rotation_angle = 0.0f;
GLfloat translation_xy[] = {0.0f, 0.0f};
static float dir_x = 1.0f;
static float dir_y = 1.0f;

/* glsl variables */
GLuint gProgram;
GLuint gvPositionHandle, gvColorHandle;
static GLuint texture, rotation_uniform, tran_uniform;
const GLfloat * vertex_data;
const GLfloat * color_data;
GLint num_vertex;

/* seed for quad/triangle */
GLint quad_num = 4;
const GLfloat quad[] = {
					-0.5f,  0.5f, 0.0f, 1.0f,
					-0.5f, -0.5f, 0.0f, 1.0f,
        				 0.5f,  0.5f, 0.0f, 1.0f,
					 0.5f, -0.5f, 0.0f, 1.0f
};
const GLfloat color_quad[] = {           0.0f, 0.0f, 0.0f, 0.0f,
					 0.0f, 0.0f, 0.0f, 0.0f,
					 0.0f, 0.0f, 0.0f, 0.0f,
					 0.0f, 0.0f, 0.0f, 0.0f };

GLint triangle_num = 3;
const GLfloat triangle[] = {
					-0.125f, -0.125f, 0.0f, 0.5f,
					 0.0f,  0.125f, 0.0f, 0.5f,
					 0.125f, -0.125f, 0.0f, 0.5f
};
const GLfloat color_triangle[] = {      
					 0.0f, 0.0f, 1.0f, 1.0f,
					 0.0f, 1.0f, 0.0f, 1.0f,
					 1.0f, 0.0f, 0.0f, 0.0f,
				 };

/* glsl program */
static const char gVertexShader[] =
    "attribute vec4 vPosition;\n"
    "attribute vec4 vColor;\n"
    "uniform float angle;\n"
    "uniform vec2 translate;\n"
    "varying vec2 texcoord;\n"
    "varying vec4 colorinfo;\n"
    "void main() {\n"
    "  float rot_angle = angle;\n"
    "  mat3 rot_z = mat3( vec3( cos(rot_angle),  sin(rot_angle), 0.0),\n"
    "                     vec3(-sin(rot_angle),  cos(rot_angle), 0.0),\n"
    "                     vec3(       0.0,         0.0, 1.0));\n"
    "  vec4 tran_vec = vec4(translate.x, translate.y ,0.0, 0.0);\n"
    "  gl_Position = vec4(rot_z * vPosition.xyz, 1.0) + tran_vec;\n"

    "  texcoord = vPosition.xy * vec2(0.5) + vec2(0.5);\n"
    "  colorinfo = vColor;\n"
    "}\n";

static const char gFragmentShader[] = "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 texcoord;\n"
    "varying vec4 colorinfo;\n"
    "void main() {\n"
    "  gl_FragColor = colorinfo + texture2D(tex, texcoord);\n"
    "}\n";

/* util functions */
GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    fprintf(stderr, "Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    } else {
	printf("bad with glshadercreatir\n");
	printf("glgeterror: %i\n", glGetError());
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
	if (!vertexShader) {
		printf("return vshad\n");
		return 0;
	}

	GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
	if (!pixelShader) {
		printf("return pshad\n");
		return 0;
	}

	GLuint program = glCreateProgram();
	if (program) {
		glAttachShader(program, vertexShader);
		glAttachShader(program, pixelShader);
		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
			if (bufLength) {
				char* buf = (char*) malloc(bufLength);
				if (buf) {
					glGetProgramInfoLog(program, bufLength, NULL, buf);
					fprintf(stderr, "Could not link program:\n%s\n", buf);
					free(buf);
				}
			}
			glDeleteProgram(program);
			program = 0;
		}
	}
	return program;
}





int setup_render_quad() {

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

	vertex_data = quad;
	num_vertex = quad_num;
	color_data = color_quad;

	glClearColor(0.0f, 0.0f, 0.5f, 1.0f); 
	return 1;
}

int setup_render_triangle() {
	vertex_data = triangle;
	color_data = color_triangle;
	num_vertex = triangle_num;
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	 
	return 1;
}

void setupGraphics(int w, int h) {
	gProgram = createProgram(gVertexShader, gFragmentShader);
	if (!gProgram) {
		printf("error making program\n");
	}
	gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
	gvColorHandle = glGetAttribLocation(gProgram, "vColor");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

	rotation_uniform = glGetUniformLocation(gProgram, "angle");
	tran_uniform = glGetUniformLocation(gProgram, "translate");
	glViewport(0, 0, w, h);    
}

void renderCompFrame(EGLDisplay d, float *x, float *y, EGLClientBuffer* window_buffers, int num_buffers) {
	int i;

	glUseProgram(gProgram); 

	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


	GLfloat tran_fixed[2];
	glUniform1fv(rotation_uniform,1, &rotation_angle);

	glActiveTexture(GL_TEXTURE0);
	glVertexAttribPointer(gvColorHandle,    num_vertex, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, color_data);  
	glVertexAttribPointer(gvPositionHandle, num_vertex, GL_FLOAT, GL_FALSE, 0, vertex_data);  
	glEnableVertexAttribArray(gvPositionHandle);
	glEnableVertexAttribArray(gvColorHandle);


	for(i=0; i<num_buffers; i++) {
		tran_fixed[0] = x[i];
		tran_fixed[1] = y[i]; 
		glUniform2fv(tran_uniform,    1, tran_fixed); 
		update_texture_khr(d, (EGLClientBuffer) window_buffers[i]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vertex);
	}

	glDisableVertexAttribArray(gvPositionHandle);
	glDisableVertexAttribArray(gvColorHandle);
}

void renderFrame(float x, float y) { 
	glUseProgram(gProgram); 

	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


	GLfloat tran_fixed[2];
	tran_fixed[0] = x;
	tran_fixed[1] = y; 

	glUniform1fv(rotation_uniform,1, &rotation_angle);
	glUniform2fv(tran_uniform,    1, tran_fixed); 

	glActiveTexture(GL_TEXTURE0);
	glVertexAttribPointer(gvColorHandle,    num_vertex, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, color_data);  
	glVertexAttribPointer(gvPositionHandle, num_vertex, GL_FLOAT, GL_FALSE, 0, vertex_data);  
	glEnableVertexAttribArray(gvPositionHandle);
	glEnableVertexAttribArray(gvColorHandle);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vertex); 
	glDisableVertexAttribArray(gvPositionHandle);
	glDisableVertexAttribArray(gvColorHandle);
}
 

void update_quad() {

	float rate_x = 0.008f;
	float rate_y = 0.0016f;
        float bound = 0.5f;
	translation_xy[0] += dir_x * rate_x;
        if ((translation_xy[0] > bound) || (translation_xy[0] < -1.0f*bound))
		dir_x *= -1.0f; 

	translation_xy[1] += dir_y * rate_y;
        if ((translation_xy[1] > bound) || (translation_xy[1] < -1.0f*bound))
		dir_y *= -1.0f; 
	return;
}

void update_triangle(void) {
	rotation_angle += 0.01f;
	return;
}

int main(int argc, char* argv[])
{
    int arg;
    opterr = 0;
    while ((arg = getopt (argc, argv, "hf:")) != -1)
    {
        switch (arg)
        {
        case 'f':
            socket_file = optarg;
            break;

        case '?':
        case 'h':
        default:
            puts(argv[0]);
            puts("Usage:");
            puts("    -f <socket filename>");
            puts("    -h: this help text");
            return -1;
        }
    }

    puts("Starting");

    mir_wait_for(mir_connect(socket_file, __PRETTY_FUNCTION__, set_connection, 0));
    puts("Connected");

    assert(connection != NULL);
    assert(mir_connection_is_valid(connection));
    assert(strcmp(mir_connection_get_error_message(connection), "") == 0);

    MirPlatformPackage platform_package;
    platform_package.data_items = -1;
    platform_package.fd_items = -1;

    MirSurfaceParameters const request_params =
        {__PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};
    mir_wait_for(mir_surface_create(connection, &request_params, surface_create_callback, 0));
    puts("Surface created");

    assert(surface != NULL);
    assert(mir_surface_is_valid(surface));
    assert(strcmp(mir_surface_get_error_message(surface), "") == 0);

	int major, minor, n;
	EGLDisplay disp;
    EGLContext context;
    EGLSurface egl_surface;
	EGLConfig egl_config;
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_GREEN_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE };
    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    EGLNativeWindowType native_window = mir_get_egl_type(surface);
   
	disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(disp, &major, &minor);
	
	eglChooseConfig(disp, attribs, &egl_config, 1, &n);
    egl_surface = eglCreateWindowSurface(disp, egl_config, native_window, NULL);
    context = eglCreateContext(disp, egl_config, EGL_NO_CONTEXT, context_attribs);
    eglMakeCurrent(disp, egl_surface, egl_surface, context);

    setupGraphics(640, 480);
	setup_render_triangle();
    while (1)
    {

		renderFrame(0.0, 0.0);
		eglSwapBuffers(disp, egl_surface);
        usleep(1667);
        update_triangle();

    }

    mir_wait_for(mir_surface_release(surface, surface_release_callback, 0));
    puts("Surface released");

    mir_connection_release(connection);
    puts("Connection released");

    return 0;
}



