#ifndef PLATFORM_GRAPHICS_EGL_DEBUG_H
#define PLATFORM_GRAPHICS_EGL_DEBUG_H

#include "mir/graphics/egl_extensions.h"
#include <GLES2/gl2.h>

#include <fstream>
#include <utility>
#include <vector>

#define MIR_LOG_COMPONENT "egl-debug"
#include "mir/log.h"

template<GLuint (*allocator)(void), void (*deleter)(GLuint)>
class GLHandle
{
public:
    GLHandle()
        : id{allocator()}
    {
    }

    ~GLHandle()
    {
        if (id)
            deleter(id);
    }

    GLHandle(GLHandle const&) = delete;
    GLHandle& operator=(GLHandle const&) = delete;
    GLHandle& operator=(GLHandle&& rhs)
    {
        std::swap(id, rhs.id);
        return *this;
    }

    GLHandle(GLHandle&& from)
        : id{std::exchange(from.id, 0)}
    {
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};

template<void (*allocator)(GLsizei, GLuint*), void (* deleter)(GLsizei, GLuint const*)>
class GLBulkHandle
{
public:
    GLBulkHandle()
    {
        allocator(1, &id);
    }

    ~GLBulkHandle()
    {
        if (id)
            deleter(1, &id);
    }

    GLBulkHandle(GLBulkHandle const&) = delete;
    GLBulkHandle& operator=(GLBulkHandle const&) = delete;
    GLBulkHandle& operator=(GLBulkHandle&& rhs)
    {
        std::swap(id, rhs.id);
        return *this;
    }

    GLBulkHandle(GLBulkHandle&& from)
        : id{std::exchange(from.id, 0)}
    {
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};
using TextureHandle = GLBulkHandle<&glGenTextures, &glDeleteTextures>;
using GLBufferHandle = GLBulkHandle<&glGenBuffers, &glDeleteBuffers>;

using RenderbufferHandle = GLBulkHandle<&glGenRenderbuffers, &glDeleteRenderbuffers>;
using FramebufferHandle = GLBulkHandle<&glGenFramebuffers, &glDeleteFramebuffers>;
using ProgramHandle = GLHandle<&glCreateProgram, &glDeleteProgram>;

bool eglimage_to_ppm(auto _glEGLImageTargetTexture2DOES, EGLImageKHR image, int width, int height, GLenum glFormat, std::string const& filename)
{
    // 1. Create an OpenGL texture and bind the EGL image to it.
    TextureHandle texture_id;
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Set texture parameters to avoid "incomplete texture" warnings or errors
    // during framebuffer attachment or glReadPixels.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // This is the core step: associating the EGLImageKHR with an OpenGL texture.
    // Note: glEGLImageTargetTexture2DOES is part of GL_OES_EGL_image extension,
    // which is widely supported where EGLImage is used with GLES.
    _glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

    // Check for OpenGL errors after binding.
    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR)
    {
        mir::log_error("Error: OpenGL error after glEGLImageTargetTexture2DOES: %X", gl_error);
        return false;
    }

    // 2. Create a Framebuffer Object (FBO) to read from the texture.
    FramebufferHandle fbo_id;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);

    // Attach the texture to the FBO's color attachment point.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);

    // Check FBO completeness status.
    GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fb_status != GL_FRAMEBUFFER_COMPLETE)
    {
        mir::log_error("Error: Framebuffer incomplete! Status: %X", fb_status);
        return false;
    }

    // Determine bytes per pixel based on format.
    int bytesPerPixel = 0;
    if (glFormat == GL_RGB)
    {
        bytesPerPixel = 3;
    }
    else if (glFormat == GL_RGBA)
    {
        bytesPerPixel = 4;
    }
    else
    {
        mir::log_error("Error: Unsupported OpenGL format for PPM. Only GL_RGB and GL_RGBA are supported.");
        return false;
    }

    // 3. Read pixels into a CPU buffer.
    // The GL_UNSIGNED_BYTE type is commonly used for 8-bit per channel images.
    std::vector<unsigned char> pixelBuffer(width * height * bytesPerPixel);
    glReadPixels(0, 0, width, height, glFormat, GL_UNSIGNED_BYTE, pixelBuffer.data());

    gl_error = glGetError();
    if (gl_error != GL_NO_ERROR)
    {
        mir::log_error("Error: OpenGL error after glReadPixels: %X", gl_error);
        return false;
    }

    // 4. Write the CPU buffer to a PPM file (P6 format for binary RGB).
    std::ofstream ofs{filename, std::ios::binary};
    if (!ofs.is_open())
    {
        mir::log_error("Error: Could not open file %s for writing", filename.c_str());
        return false;
    }

    // Write PPM header
    // P6: Magic number for binary RGB PPM
    // width height: Image dimensions
    // 255: Max color value (8-bit per channel)
    ofs << "P6\n" << width << " " << height << "\n255\n";

    // Write pixel data. PPM P6 expects RGB byte triplets.
    // glReadPixels reads from the bottom-left, so we need to flip the image vertically for common display.
    // PPM stores pixels from top-left, row by row.
    for (int y = height - 1; y >= 0; --y)
    { // Iterate from bottom row to top row (OpenGL's origin)
        for (int x = 0; x < width; ++x)
        {
            int index = (y * width + x) * bytesPerPixel;
            ofs.put(pixelBuffer[index]);     // Red
            ofs.put(pixelBuffer[index + 1]); // Green
            ofs.put(pixelBuffer[index + 2]); // Blue

            // If it's RGBA, skip the alpha channel for RGB PPM.
            // If you need alpha, you'd save to a different format (e.g., PNG).
        }
    }

    ofs.close();

    mir::log_debug("Successfully saved EGL image to %s", filename.c_str());
    return true;
}

#endif // PLATFORM_GRAPHICS_EGL_DEBUG_H
