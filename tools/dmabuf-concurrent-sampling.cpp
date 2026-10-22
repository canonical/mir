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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <barrier>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace
{
constexpr unsigned buffer_count{3};

constexpr char help[] = R"(Usage: dmabuf-concurrent-sampling RENDER_NODE [options]

Standalone GPU probe for https://github.com/canonical/mir/issues/5325.
No compositor, display server or DRM master is needed.

Build from the Mir source directory:
  c++ -std=c++23 -O2 -Wall -Wextra -Wpedantic -pthread \
      tools/dmabuf-concurrent-sampling.cpp \
      $(pkg-config --cflags --libs egl glesv2 gbm libdrm) \
      -o build/dmabuf-concurrent-sampling

Options:
  --threads N          Consumer contexts/threads (default 3, maximum 64)
  --frames N           Frames to check (default 1000)
  --width N            Full producer buffer width (default 5760)
  --height N           Buffer height (default 1080)
  --draws N            Full-buffer producer draws per frame (default 16)
  --serial             Serialize consumer submission AND GPU completion
  --producer-finish    Finish producer rendering before notifying consumers
  --modifier HEX       Require this exact GBM modifier; no allocation fallback
  --help               Show this text

Compare these runs on the affected device, keeping other arguments unchanged:
  build/dmabuf-concurrent-sampling /dev/dri/renderD128
  build/dmabuf-concurrent-sampling /dev/dri/renderD128 --serial
  build/dmabuf-concurrent-sampling /dev/dri/renderD128 --producer-finish
  build/dmabuf-concurrent-sampling /dev/dri/renderD128 --threads 1

On the affected Intel hardware also try --modifier 0x0100000000000004
(I915_FORMAT_MOD_Y_TILED_CCS) and --modifier 0 (linear). Check the printed
modifier value, vendor, name and plane count; the default need not use compression.
The chosen format/modifier must support renderable GL_TEXTURE_2D imports.

The processes fork BEFORE opening independent GPU devices/contexts. A producer
allocates three ABGR8888 GBM buffers and transfers every plane FD with SCM_RIGHTS.
The consumer imports each buffer ONCE into a texture shared by its EGL contexts.
For each frame the producer draws a frame/position-dependent byte-exact pattern,
glFlushes (NOT glFinishes), and sends the frame number, not a synchronization FD.
Workers sample disjoint horizontal slices of that SAME texture into PRIVATE
FBOs. Concurrent workers rendezvous before drawing and again after submission,
before any glReadPixels. Every output pixel is checked.

Readback waits only for each private output to complete; it does not repair an
already incorrect sample. The producer waits for all readbacks before reuse,
so it never intentionally writes a buffer still being sampled. Startup glFinish
calls publish initialized resources before testing, not per-frame synchronization.

Concurrent-only failures that disappear with --serial support a concurrent-
consumer problem. Disappearance with --producer-finish implicates ordering or
visibility of producer writes, but neither outcome alone assigns blame to Mesa.
These controls also change timing; a lower failure rate alone is not conclusive.
A clean run does not disprove the issue: workload, modifier, driver scheduling,
readback, and this one-frame-in-flight protocol differ from a real compositor.
No software-renderer or unsupported-modifier fallback is performed.

Exit codes: 0 = all pixels correct, 1 = pixel mismatches, 2 = setup/runtime error.
)";

struct Options
{
    std::string node;
    unsigned threads{3};
    unsigned frames{1000};
    unsigned width{5760};
    unsigned height{1080};
    unsigned draws{16};
    bool serial{false};
    bool producer_finish{false};
    std::optional<uint64_t> modifier;
};

auto number(std::string_view text, int base = 10) -> uint64_t
{
    if (base == 16 && text.starts_with("0x"))
        text.remove_prefix(2);
    uint64_t value{};
    auto const [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (error != std::errc{} || end != text.data() + text.size() || text.empty())
        throw std::runtime_error(std::format("Invalid number: {}", text));
    return value;
}

auto options(int argc, char** argv) -> std::optional<Options>
{
    Options result;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view const arg{argv[i]};
        if (arg == "--help")
        {
            std::printf("%s", help);
            return std::nullopt;
        }
        if (arg == "--serial")
            result.serial = true;
        else if (arg == "--producer-finish")
            result.producer_finish = true;
        else if (!arg.starts_with("--") && result.node.empty())
            result.node = arg;
        else
        {
            if (i + 1 == argc)
                throw std::runtime_error(std::format("Missing value or unknown argument: {}", arg));
            std::string_view const value{argv[++i]};
            if (arg == "--modifier")
                result.modifier = number(value, 16);
            else
            {
                auto const parsed = number(value);
                if (parsed == 0 || parsed > 1'000'000)
                    throw std::runtime_error("Numeric options must be between 1 and 1000000");
                auto const n = static_cast<unsigned>(parsed);
                if (arg == "--threads")
                    result.threads = n;
                else if (arg == "--frames")
                    result.frames = n;
                else if (arg == "--width")
                    result.width = n;
                else if (arg == "--height")
                    result.height = n;
                else if (arg == "--draws")
                    result.draws = n;
                else
                    throw std::runtime_error(std::format("Unknown argument: {}", arg));
            }
        }
    }
    if (result.node.empty() || result.threads > 64 || result.width % result.threads != 0)
        throw std::runtime_error("Specify a render node; width must be divisible by threads (1..64). See --help");
    return result;
}

void check_system(bool success, char const* operation)
{
    if (!success)
        throw std::system_error(errno, std::generic_category(), operation);
}

void check_egl(bool success, char const* operation)
{
    if (!success)
        throw std::runtime_error(std::format("{}: EGL error {:#x}", operation, eglGetError()));
}

void check_gl(char const* operation)
{
    if (auto const error = glGetError(); error != GL_NO_ERROR)
        throw std::runtime_error(std::format("{}: GL error {:#x}", operation, error));
}

class Fd
{
public:
    explicit Fd(int fd) : value{fd} { check_system(fd >= 0, "open/export file descriptor"); }
    Fd(Fd&& other) noexcept : value{std::exchange(other.value, -1)} {}
    Fd(Fd const&) = delete;
    auto operator=(Fd const&) -> Fd& = delete;
    ~Fd() { if (value >= 0) close(value); }
    auto get() const -> int { return value; }

private:
    int value;
};

template<typename T>
struct Packet
{
    T value{};
    std::vector<Fd> fds;
};

template<typename T>
void send_packet(int socket, T const& value, std::span<int const> fds = {})
{
    if (fds.size() > 4)
        throw std::runtime_error("Too many dma-buf planes");
    iovec data{const_cast<T*>(&value), sizeof(value)};
    alignas(cmsghdr) std::array<char, CMSG_SPACE(4 * sizeof(int))> control{};
    msghdr message{};
    message.msg_iov = &data;
    message.msg_iovlen = 1;
    if (!fds.empty())
    {
        message.msg_control = control.data();
        message.msg_controllen = CMSG_SPACE(fds.size_bytes());
        auto const header = CMSG_FIRSTHDR(&message);
        header->cmsg_level = SOL_SOCKET;
        header->cmsg_type = SCM_RIGHTS;
        header->cmsg_len = CMSG_LEN(fds.size_bytes());
        std::memcpy(CMSG_DATA(header), fds.data(), fds.size_bytes());
    }
    ssize_t size;
    do { size = sendmsg(socket, &message, MSG_NOSIGNAL); } while (size < 0 && errno == EINTR);
    check_system(size >= 0, "sendmsg");
    if (size != sizeof(value))
        throw std::runtime_error("Short IPC send");
}

template<typename T>
auto receive_packet(int socket) -> Packet<T>
{
    Packet<T> packet;
    iovec data{&packet.value, sizeof(packet.value)};
    alignas(cmsghdr) std::array<char, CMSG_SPACE(4 * sizeof(int))> control{};
    msghdr message{};
    message.msg_iov = &data;
    message.msg_iovlen = 1;
    message.msg_control = control.data();
    message.msg_controllen = control.size();
    ssize_t size;
    do { size = recvmsg(socket, &message, MSG_CMSG_CLOEXEC); } while (size < 0 && errno == EINTR);
    check_system(size >= 0, "recvmsg");
    for (auto header = CMSG_FIRSTHDR(&message); header; header = CMSG_NXTHDR(&message, header))
    {
        if (header->cmsg_level != SOL_SOCKET || header->cmsg_type != SCM_RIGHTS)
            throw std::runtime_error("Unexpected IPC ancillary data");
        auto const count = (header->cmsg_len - CMSG_LEN(0)) / sizeof(int);
        auto const descriptors = reinterpret_cast<int const*>(CMSG_DATA(header));
        for (size_t i = 0; i < count; ++i)
            packet.fds.emplace_back(descriptors[i]);
    }
    if (size != sizeof(T) || (message.msg_flags & (MSG_TRUNC | MSG_CTRUNC)))
        throw std::runtime_error("IPC peer disconnected or sent a truncated packet");
    return packet;
}

template<typename T>
auto receive_value(int socket) -> T
{
    auto packet = receive_packet<T>(socket);
    if (!packet.fds.empty())
        throw std::runtime_error("Unexpected IPC file descriptors");
    return packet.value;
}

auto has_extension(char const* extensions, char const* name) -> bool
{
    return extensions && std::string{" " + std::string{extensions} + " "}.contains(" " + std::string{name} + " ");
}

template<typename T>
auto entry_point(char const* name) -> T
{
    auto const function = reinterpret_cast<T>(eglGetProcAddress(name));
    if (!function)
        throw std::runtime_error(std::format("Missing EGL/GL entry point: {}", name));
    return function;
}

class Device
{
public:
    explicit Device(Options const& opts)
        : fd{open(opts.node.c_str(), O_RDWR | O_CLOEXEC)},
          gbm{gbm_create_device(fd.get()), gbm_device_destroy}
    {
        if (!gbm)
            throw std::runtime_error("gbm_create_device failed");
    }

    ~Device()
    {
        if (initialized && !eglTerminate(display))
            std::fprintf(stderr, "eglTerminate failed: %#x\n", eglGetError());
    }

    void initialize()
    {
        display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, gbm.get(), nullptr);
        check_egl(display != EGL_NO_DISPLAY, "eglGetPlatformDisplay(GBM)");
        check_egl(eglInitialize(display, nullptr, nullptr), "eglInitialize");
        initialized = true;
        auto const extensions = eglQueryString(display, EGL_EXTENSIONS);
        for (auto name : {"EGL_KHR_surfaceless_context", "EGL_EXT_image_dma_buf_import",
                          "EGL_EXT_image_dma_buf_import_modifiers"})
        {
            if (!has_extension(extensions, name))
                throw std::runtime_error(std::format("Required extension not supported: {}", name));
        }
        EGLint const attributes[]{EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, 0, EGL_NONE};
        EGLint count{};
        check_egl(eglChooseConfig(display, attributes, &config, 1, &count) && count == 1, "eglChooseConfig");
        create_image = entry_point<PFNEGLCREATEIMAGEKHRPROC>("eglCreateImageKHR");
        destroy_image = entry_point<PFNEGLDESTROYIMAGEKHRPROC>("eglDestroyImageKHR");
        image_target = entry_point<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>("glEGLImageTargetTexture2DOES");
    }

    Fd fd;
    std::unique_ptr<gbm_device, decltype(&gbm_device_destroy)> gbm;
    EGLDisplay display{EGL_NO_DISPLAY};
    EGLConfig config{};
    PFNEGLCREATEIMAGEKHRPROC create_image{};
    PFNEGLDESTROYIMAGEKHRPROC destroy_image{};
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target{};

private:
    bool initialized{false};
};

class Context
{
public:
    explicit Context(Device& device, EGLContext share = EGL_NO_CONTEXT) : device{device}
    {
        check_egl(eglBindAPI(EGL_OPENGL_ES_API), "eglBindAPI");
        EGLint const attributes[]{EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context = eglCreateContext(device.display, device.config, share, attributes);
        check_egl(context != EGL_NO_CONTEXT, "eglCreateContext");
    }

    ~Context()
    {
        if (!eglMakeCurrent(device.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
            std::fprintf(stderr, "eglMakeCurrent(release) failed: %#x\n", eglGetError());
        if (!eglDestroyContext(device.display, context))
            std::fprintf(stderr, "eglDestroyContext failed: %#x\n", eglGetError());
    }

    void make_current()
    {
        check_egl(eglMakeCurrent(device.display, EGL_NO_SURFACE, EGL_NO_SURFACE, context), "eglMakeCurrent");
    }

    Device& device;
    EGLContext context{EGL_NO_CONTEXT};
};

void describe_gpu(Options const& opts, Device& device, char const* role)
{
    auto const renderer = reinterpret_cast<char const*>(glGetString(GL_RENDERER));
    if (!renderer)
        throw std::runtime_error("No GL renderer");
    if (!has_extension(reinterpret_cast<char const*>(glGetString(GL_EXTENSIONS)), "GL_OES_EGL_image"))
        throw std::runtime_error("GL_OES_EGL_image is required");
    std::string_view const name{renderer};
    if (name.contains("llvmpipe") || name.contains("softpipe") || name.contains("Software") || name.contains("swrast"))
        throw std::runtime_error(std::format("A hardware GPU is required; renderer is {}", name));
    GLint limit{};
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    if (opts.width > static_cast<unsigned>(limit) || opts.height > static_cast<unsigned>(limit))
        throw std::runtime_error(std::format("Requested dimensions exceed GL_MAX_TEXTURE_SIZE={}", limit));
    check_gl("query GPU");
    auto const vendor = eglQueryString(device.display, EGL_VENDOR);
    check_egl(vendor != nullptr, "eglQueryString(EGL_VENDOR)");
    std::printf("%s: EGL vendor=%s; GL renderer=%s\n", role, vendor, renderer);
}

struct GlObjects
{
    GlObjects() = default;
    GlObjects(GlObjects const&) = delete;
    auto operator=(GlObjects const&) -> GlObjects& = delete;

    ~GlObjects()
    {
        if (program) glDeleteProgram(program);
        if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
        if (texture) glDeleteTextures(1, &texture);
    }

    GLuint texture{};
    GLuint framebuffer{};
    GLuint program{};
};

struct BufferInfo
{
    uint32_t width{};
    uint32_t height{};
    uint32_t format{};
    uint32_t planes{};
    uint64_t modifier{};
    std::array<uint32_t, 4> offsets{};
    std::array<uint32_t, 4> strides{};
};

auto export_buffer(gbm_bo* bo) -> Packet<BufferInfo>
{
    Packet<BufferInfo> result;
    auto& info = result.value;
    info.width = gbm_bo_get_width(bo);
    info.height = gbm_bo_get_height(bo);
    info.format = gbm_bo_get_format(bo);
    info.modifier = gbm_bo_get_modifier(bo);
    info.planes = gbm_bo_get_plane_count(bo);
    if (info.planes == 0 || info.planes > 4)
        throw std::runtime_error("Unsupported GBM plane count");
    for (unsigned i = 0; i < info.planes; ++i)
    {
        result.fds.emplace_back(gbm_bo_get_fd_for_plane(bo, i));
        info.offsets[i] = gbm_bo_get_offset(bo, i);
        info.strides[i] = gbm_bo_get_stride_for_plane(bo, i);
    }
    return result;
}

void import_texture(Device& device, Packet<BufferInfo> const& packet, GlObjects& objects)
{
    auto const& info = packet.value;
    constexpr std::array<std::array<EGLint, 5>, 4> keys{{
        {EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE0_PITCH_EXT,
         EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT},
        {EGL_DMA_BUF_PLANE1_FD_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT,
         EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT},
        {EGL_DMA_BUF_PLANE2_FD_EXT, EGL_DMA_BUF_PLANE2_OFFSET_EXT, EGL_DMA_BUF_PLANE2_PITCH_EXT,
         EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT},
        {EGL_DMA_BUF_PLANE3_FD_EXT, EGL_DMA_BUF_PLANE3_OFFSET_EXT, EGL_DMA_BUF_PLANE3_PITCH_EXT,
         EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT}}};
    if (info.planes == 0 || info.planes > 4 || packet.fds.size() != info.planes)
        throw std::runtime_error("Invalid dma-buf plane descriptors");
    std::vector<EGLint> attributes{EGL_WIDTH, static_cast<EGLint>(info.width),
        EGL_HEIGHT, static_cast<EGLint>(info.height), EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(info.format)};
    for (unsigned i = 0; i < info.planes; ++i)
    {
        attributes.insert(attributes.end(), {keys[i][0], packet.fds[i].get(),
            keys[i][1], static_cast<EGLint>(info.offsets[i]), keys[i][2], static_cast<EGLint>(info.strides[i])});
        if (info.modifier != DRM_FORMAT_MOD_INVALID)
            attributes.insert(attributes.end(), {keys[i][3], static_cast<EGLint>(info.modifier & 0xffffffff),
                keys[i][4], static_cast<EGLint>(info.modifier >> 32)});
    }
    attributes.push_back(EGL_NONE);
    auto const image = device.create_image(
        device.display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attributes.data());
    check_egl(image != EGL_NO_IMAGE_KHR, "eglCreateImageKHR(dma-buf)");
    glGenTextures(1, &objects.texture);
    glBindTexture(GL_TEXTURE_2D, objects.texture);
    device.image_target(GL_TEXTURE_2D, image);
    auto const destroyed = device.destroy_image(device.display, image);
    check_gl("import dma-buf as GL_TEXTURE_2D");
    check_egl(destroyed, "eglDestroyImageKHR");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    check_gl("configure imported texture");
}

void attach_framebuffer(GlObjects& objects)
{
    glGenFramebuffers(1, &objects.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, objects.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, objects.texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Texture is not framebuffer-renderable");
    check_gl("attach framebuffer");
}

constexpr char vertex_shader[] = R"(#version 300 es
void main()
{
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

constexpr char producer_shader[] = R"(#version 300 es
precision highp float;
precision highp int;
uniform uint key;
out vec4 color;
void main()
{
    uvec2 p = uvec2(gl_FragCoord.xy);
    uvec3 bytes = uvec3((key + p.x / 32u) & 255u,
                       ((key >> 8u) + p.y / 16u) & 255u,
                       ((key >> 16u) ^ (p.x / 64u) ^ (p.y / 64u)) & 255u);
    color = vec4(vec3(bytes) / 255.0, 1.0);
}
)";

constexpr char consumer_shader[] = R"(#version 300 es
precision highp float;
precision highp int;
uniform highp sampler2D source;
uniform int offset;
out vec4 color;
void main()
{
    color = texelFetch(source, ivec2(gl_FragCoord.xy) + ivec2(offset, 0), 0);
}
)";

class Shader
{
public:
    Shader(GLenum type, char const* source) : id{glCreateShader(type)}
    {
        if (!id)
            throw std::runtime_error("glCreateShader failed");
        glShaderSource(id, 1, &source, nullptr);
        glCompileShader(id);
    }
    ~Shader() { glDeleteShader(id); }

    void check() const
    {
        GLint success{};
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::array<char, 4096> log{};
            glGetShaderInfoLog(id, log.size(), nullptr, log.data());
            throw std::runtime_error(std::format("Shader compilation failed: {}", log.data()));
        }
    }

    GLuint const id;
};

void make_program(GlObjects& objects, char const* fragment_source)
{
    Shader vertex{GL_VERTEX_SHADER, vertex_shader};
    Shader fragment{GL_FRAGMENT_SHADER, fragment_source};
    vertex.check();
    fragment.check();
    objects.program = glCreateProgram();
    glAttachShader(objects.program, vertex.id);
    glAttachShader(objects.program, fragment.id);
    glLinkProgram(objects.program);
    GLint success{};
    glGetProgramiv(objects.program, GL_LINK_STATUS, &success);
    if (!success)
    {
        std::array<char, 4096> log{};
        glGetProgramInfoLog(objects.program, log.size(), nullptr, log.data());
        throw std::runtime_error(std::format("Program linking failed: {}", log.data()));
    }
    glUseProgram(objects.program);
    check_gl("create shader program");
}

auto uniform(GLuint program, char const* name) -> GLint
{
    auto const location = glGetUniformLocation(program, name);
    if (location == -1)
        throw std::runtime_error(std::format("Missing shader uniform: {}", name));
    return location;
}

auto pattern_key(unsigned frame, unsigned pass) -> uint32_t
{
    return uint32_t{frame} * 1664525u + uint32_t{pass} * 1013904223u;
}

auto expected_pixel(uint32_t key, unsigned x, unsigned y) -> std::array<uint8_t, 4>
{
    return {static_cast<uint8_t>((key + x / 32u) & 255u),
        static_cast<uint8_t>(((key >> 8u) + y / 16u) & 255u),
        static_cast<uint8_t>(((key >> 16u) ^ (x / 64u) ^ (y / 64u)) & 255u), 255};
}

struct SampleResult
{
    std::exception_ptr error;
    uint64_t bad_pixels{};
    unsigned x{};
    unsigned y{};
    std::array<uint8_t, 4> expected{};
    std::array<uint8_t, 4> actual{};
};

auto check_pixels(
    std::span<uint8_t const> pixels, unsigned width, unsigned height, unsigned offset, uint32_t key) -> SampleResult
{
    if (pixels.size() != size_t{width} * height * 4)
        throw std::runtime_error("Incorrect readback size");
    SampleResult result;
    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width; ++x)
        {
            auto const expected = expected_pixel(key, x + offset, y);
            auto const pixel = pixels.subspan((size_t{y} * width + x) * 4, 4);
            if (!std::equal(expected.begin(), expected.end(), pixel.begin()))
            {
                if (result.bad_pixels++ == 0)
                {
                    result.x = x + offset;
                    result.y = y;
                    result.expected = expected;
                    std::copy(pixel.begin(), pixel.end(), result.actual.begin());
                }
            }
        }
    }
    return result;
}

class ProducerBuffer
{
public:
    ProducerBuffer(Device& device, Options const& opts)
        : bo{opts.modifier ?
              gbm_bo_create_with_modifiers2(device.gbm.get(), opts.width, opts.height, DRM_FORMAT_ABGR8888,
                  &*opts.modifier, 1, GBM_BO_USE_RENDERING) :
              gbm_bo_create(device.gbm.get(), opts.width, opts.height, DRM_FORMAT_ABGR8888, GBM_BO_USE_RENDERING),
              gbm_bo_destroy}
    {
        if (!bo)
            throw std::runtime_error("GBM buffer allocation failed (requested format/modifier may be unsupported)");
        packet = export_buffer(bo.get());
        if (opts.modifier && packet.value.modifier != *opts.modifier)
            throw std::runtime_error("GBM did not allocate the requested modifier");
        import_texture(device, packet, objects);
        attach_framebuffer(objects);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        check_gl("initialize producer buffer");
    }

    std::unique_ptr<gbm_bo, decltype(&gbm_bo_destroy)> bo;
    Packet<BufferInfo> packet;
    GlObjects objects;
};

class Sampler
{
public:
    Sampler(Device& device, EGLContext share, Options const& opts, unsigned index)
        : context{device, share}, width{opts.width / opts.threads}, height{opts.height}, offset{index * width},
          pixels(size_t{width} * height * 4)
    {
        context.make_current();
        glDisable(GL_DITHER);
        glGenTextures(1, &objects.texture);
        glBindTexture(GL_TEXTURE_2D, objects.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        attach_framebuffer(objects);
        make_program(objects, consumer_shader);
        glUniform1i(uniform(objects.program, "source"), 0);
        glUniform1i(uniform(objects.program, "offset"), offset);
        glViewport(0, 0, width, height);
        check_gl("initialize consumer");
    }

    void draw(GLuint source)
    {
        glBindTexture(GL_TEXTURE_2D, source);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glFlush();
        check_gl("submit sample");
    }

    auto read(unsigned frame, unsigned draws) -> SampleResult
    {
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        check_gl("read private consumer framebuffer");
        return check_pixels(pixels, width, height, offset, pattern_key(frame, draws - 1));
    }

private:
    Context context;
    GlObjects objects;
    unsigned const width;
    unsigned const height;
    unsigned const offset;
    std::vector<uint8_t> pixels;
};

class SamplingGroup
{
public:
    SamplingGroup(
        Device& device, EGLContext share, Options const& opts, std::array<GlObjects, buffer_count> const& sources)
        : device{device}, share{share}, opts{opts}, sources{sources},
          phase{opts.threads + 1}, submitted{opts.threads}, results(opts.threads)
    {
        workers.reserve(opts.threads);
    }

    ~SamplingGroup()
    {
        if (!started)
        {
            for (size_t i = workers.size(); i < opts.threads; ++i)
            {
                phase.arrive_and_drop();
                submitted.arrive_and_drop();
            }
            phase.arrive_and_wait();
        }
        stop = true;
        phase.arrive_and_wait();
        for (auto& worker : workers)
            worker.join();
    }

    void start()
    {
        for (unsigned i = 0; i < opts.threads; ++i)
            workers.emplace_back([this, i] { sample_loop(i); });
        phase.arrive_and_wait();
        started = true;
        check_errors();
    }

    auto sample(unsigned next_frame) -> std::vector<SampleResult> const&
    {
        frame = next_frame;
        phase.arrive_and_wait();
        phase.arrive_and_wait();
        check_errors();
        return results;
    }

private:
    void check_errors() const
    {
        for (auto const& result : results)
            if (result.error)
                std::rethrow_exception(result.error);
    }

    void sample_loop(unsigned index)
    {
        std::unique_ptr<Sampler> sampler;
        try { sampler = std::make_unique<Sampler>(device, share, opts, index); }
        catch (...) { results[index].error = std::current_exception(); }
        phase.arrive_and_wait();
        while (true)
        {
            phase.arrive_and_wait();
            if (stop)
                break;
            auto const source = sources[(frame - 1) % buffer_count].texture;
            if (opts.serial)
            {
                std::lock_guard lock{serial_mutex};
                if (!results[index].error)
                {
                    try
                    {
                        sampler->draw(source);
                        results[index] = sampler->read(frame, opts.draws);
                    }
                    catch (...) { results[index].error = std::current_exception(); }
                }
            }
            else
            {
                if (!results[index].error)
                {
                    try { sampler->draw(source); }
                    catch (...) { results[index].error = std::current_exception(); }
                }
                // No readback can accidentally finish GPU work before all consumers submit.
                submitted.arrive_and_wait();
                if (!results[index].error)
                {
                    try { results[index] = sampler->read(frame, opts.draws); }
                    catch (...) { results[index].error = std::current_exception(); }
                }
            }
            phase.arrive_and_wait();
        }
    }

    Device& device;
    EGLContext const share;
    Options const& opts;
    std::array<GlObjects, buffer_count> const& sources;
    std::barrier<> phase;
    std::barrier<> submitted;
    std::vector<SampleResult> results;
    std::vector<std::thread> workers;
    std::mutex serial_mutex;
    bool started{false};
    bool stop{false};
    unsigned frame{};
};

struct Ack
{
    uint32_t frame{};
    uint32_t bad_samples{};
};

auto consume(Options const& opts, int socket) -> int
{
    Device device{opts};
    device.initialize();
    Context root{device};
    root.make_current();
    describe_gpu(opts, device, "consumer");
    std::array<GlObjects, buffer_count> sources;
    for (auto& source : sources)
    {
        auto packet = receive_packet<BufferInfo>(socket);
        if (packet.value.width != opts.width || packet.value.height != opts.height ||
            packet.value.format != DRM_FORMAT_ABGR8888)
            throw std::runtime_error("Unexpected producer buffer dimensions/format");
        import_texture(device, packet, source);
    }
    glFinish();
    check_gl("publish shared source textures");
    SamplingGroup group{device, root.context, opts, sources};
    group.start();
    send_packet(socket, Ack{});
    uint64_t bad_frames{};
    uint64_t bad_samples{};
    auto const start = std::chrono::steady_clock::now();
    for (unsigned frame = 1; frame <= opts.frames; ++frame)
    {
        if (receive_value<uint32_t>(socket) != frame)
            throw std::runtime_error("Unexpected producer frame number");
        Ack ack{frame, 0};
        auto const& results = group.sample(frame);
        for (unsigned i = 0; i < results.size(); ++i)
        {
            auto const& result = results[i];
            if (result.bad_pixels)
            {
                ++ack.bad_samples;
                if (++bad_samples <= 10)
                {
                    std::fprintf(stderr, "MISMATCH frame=%u buffer=%u worker=%u bad_pixels=%llu "
                        "first=(%u,%u) expected=(%d,%d,%d,%d) actual=(%d,%d,%d,%d)\n",
                        frame, (frame - 1) % buffer_count, i, static_cast<unsigned long long>(result.bad_pixels),
                        result.x, result.y,
                        result.expected[0], result.expected[1], result.expected[2], result.expected[3],
                        result.actual[0], result.actual[1], result.actual[2], result.actual[3]);
                }
            }
        }
        bad_frames += ack.bad_samples != 0;
        send_packet(socket, ack);
    }
    auto const seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::printf("%s: frames=%u samples=%llu bad_frames=%llu bad_samples=%llu elapsed=%.2fs\n",
        bad_samples ? "FAIL" : "PASS", opts.frames,
        static_cast<unsigned long long>(uint64_t{opts.frames} * opts.threads),
        static_cast<unsigned long long>(bad_frames), static_cast<unsigned long long>(bad_samples), seconds);
    return bad_samples ? 1 : 0;
}

void produce(Options const& opts, int socket)
{
    Device device{opts};
    device.initialize();
    Context context{device};
    context.make_current();
    describe_gpu(opts, device, "producer");
    glDisable(GL_DITHER);
    GlObjects program;
    make_program(program, producer_shader);
    auto const key_uniform = uniform(program.program, "key");
    std::array<std::unique_ptr<ProducerBuffer>, buffer_count> buffers;
    for (auto& buffer : buffers)
        buffer = std::make_unique<ProducerBuffer>(device, opts);
    glFinish();
    check_gl("initialize producer pool");
    for (unsigned i = 0; i < buffers.size(); ++i)
    {
        auto const& packet = buffers[i]->packet;
        std::vector<int> fds;
        for (auto const& fd : packet.fds)
            fds.push_back(fd.get());
        std::unique_ptr<char, decltype(&std::free)> const vendor{
            drmGetFormatModifierVendor(packet.value.modifier), &std::free};
        std::unique_ptr<char, decltype(&std::free)> const name{
            drmGetFormatModifierName(packet.value.modifier), &std::free};
        std::printf("buffer=%u format=ABGR8888 modifier=%#018llx (%s:%s) planes=%u\n",
            i, static_cast<unsigned long long>(packet.value.modifier),
            vendor ? vendor.get() : "(UNKNOWN VENDOR)", name ? name.get() : "(UNKNOWN MODIFIER)", packet.value.planes);
        send_packet(socket, packet.value, fds);
    }
    if (auto const ack = receive_value<Ack>(socket); ack.frame != 0 || ack.bad_samples != 0)
        throw std::runtime_error("Invalid consumer startup acknowledgement");
    glViewport(0, 0, opts.width, opts.height);
    for (unsigned frame = 1; frame <= opts.frames; ++frame)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, buffers[(frame - 1) % buffer_count]->objects.framebuffer);
        for (unsigned pass = 0; pass < opts.draws; ++pass)
        {
            glUniform1ui(key_uniform, pattern_key(frame, pass));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glFlush();
        if (opts.producer_finish)
            glFinish();
        check_gl("submit producer frame");
        send_packet(socket, uint32_t{frame});
        auto const ack = receive_value<Ack>(socket);
        if (ack.frame != frame || ack.bad_samples > opts.threads)
            throw std::runtime_error("Invalid consumer frame acknowledgement");
    }
}

class Child
{
public:
    explicit Child(pid_t pid) : pid{pid} {}
    ~Child()
    {
        if (pid > 0)
        {
            if (kill(pid, SIGTERM) < 0 && errno != ESRCH)
                std::perror("kill consumer");
            while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {}
        }
    }

    auto wait() -> int
    {
        int status{};
        pid_t result;
        do { result = waitpid(pid, &status, 0); } while (result < 0 && errno == EINTR);
        check_system(result == pid, "waitpid");
        pid = -1;
        if (!WIFEXITED(status))
            throw std::runtime_error(std::format("Consumer terminated by signal {}", WTERMSIG(status)));
        return WEXITSTATUS(status);
    }

private:
    pid_t pid;
};
}

int main(int argc, char** argv)
{
    char const* role = "producer";
    try
    {
        auto const opts = options(argc, argv);
        if (!opts)
            return 0;
        setvbuf(stdout, nullptr, _IOLBF, 0);
        std::printf("node=%s size=%ux%u threads=%u frames=%u draws=%u mode=%s producer_finish=%s\n",
            opts->node.c_str(), opts->width, opts->height, opts->threads, opts->frames, opts->draws,
            opts->serial ? "serial" : "concurrent", opts->producer_finish ? "yes" : "no");
        int sockets[2];
        check_system(socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets) == 0, "socketpair");
        auto const pid = fork();
        if (pid < 0)
        {
            auto const error = errno;
            close(sockets[0]);
            close(sockets[1]);
            throw std::system_error(error, std::generic_category(), "fork");
        }
        if (pid == 0)
        {
            role = "consumer";
            close(sockets[0]);
            Fd socket{sockets[1]};
            return consume(*opts, socket.get());
        }
        close(sockets[1]);
        Fd socket{sockets[0]};
        Child child{pid};
        produce(*opts, socket.get());
        return child.wait();
    }
    catch (std::exception const& error)
    {
        std::fprintf(stderr, "%s: %s\n", role, error.what());
        return 2;
    }
}
