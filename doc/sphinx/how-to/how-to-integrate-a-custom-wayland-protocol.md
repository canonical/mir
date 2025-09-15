# How to integrate a custom Wayland protocol

Mir enables shells to add custom Wayland protocols to their Mir-based
compositors.

**Note**: [Write Your First Wayland
Compositor](../tutorial/write-your-first-wayland-compositor.md) is a prerequisite for
this how-to.

______________________________________________________________________

First, have a Wayland protocol XML on hand. As an example, we will use
the ["ext-workspace-v1"](https://wayland.app/protocols/ext-workspace-v1)
protocol.

Next, if necessary, install the `mirwayland` library. On Fedora and Alpine
it is already installed as part of the `mir-devel`/`mir-dev` package. On
Debian and its derivatives, this can be installed with: `sudo apt install libmirwayland-dev`.

Afterwards, let's integrate our Wayland protocol XML into our project's build.
To do this, we place `ext-workspace-v1.xml` into the `wayland-protocols`
directory. Then we modify our project's `CMakeLists.txt` file to build our
protocol XML into a corresponding header and source file:

```cmake
# CMakeLists.txt

set(PROTOCOL "ext-workspace-v1")
set(PROTOCOL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/wayland-protocols/${PROTOCOL}.xml")
set(GENERATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ext-workspace-v1_wrapper")

add_custom_command(
    OUTPUT ${GENERATE_FILE}.cpp
    OUTPUT ${GENERATE_FILE}.h
    DEPENDS ${PROTOCOL_FILE}
    COMMAND "sh" "-c" "mir_wayland_generator zwp_ ${PROTOCOL_FILE} header >${GENERATE_FILE}.h"
    COMMAND "sh" "-c" "mir_wayland_generator zwp_ ${PROTOCOL_FILE} source >${GENERATE_FILE}.cpp"
)
```

Next, rerun `cmake` for your project. Note that the following two files will appear in
the filesystem:

1. `wayland-generated/ext-workspace-v1_wrapper.h`
1. `wayland-generated/ext-workspace-v1_wrapper.cpp`

You may choose to integrate the protocol files in a separate library
or you may include the source files directly into your build. This step is up
to you and depends on how you are structuring your project.

Now that we have the files included in our build, we can integrate this new
protocol extension into our compositor. To do this, we use `miral::WaylandExtensions::add_extension`.
This method expects a `miral::WaylandExtensions::Builder` argument to be passed to it.
In addition to the `name` of the protocol, this argument provides a `build` function
for creating the global for the protocol.

As an example, let's enable our `ext-workspace-v1` protocol. First, we add our
extension builder to the list available extensions:

```cpp
// main.cpp
#include "ext-workspace-v1.h"  // Defines build_ext_workspace_v1_global

/// ...

int main()
{
    /// ....
    miral::WaylandExtensions extensions;

    extensions.add_extension(build_ext_workspace_v1_global)

    ///
    return runner.run_with({
        /// ...
        extensions
    });
}
```

Next, we implement `build_ext_workspace_v1_global`. This method will return
a builder method whose return value is a `Global` of some sort. The `Global` is a C++
class that individual Wayland clients "bind" to. When a client binds to an interface,
the compositor is expected to instantiate a new instance of that Wayland object.
As an example, here is a stub implementation of the builder for `ext-workspace-v1`:

```cpp
// ext_workspace_v1.cpp
#include "ext-workspace-v1_wrapper.h"

/// This class implements the Global interface generated for the protocol.
/// The Global is responsible for instantiating resources when a client binds
/// to the protocol. These resources will be automatically deconstructed later on.
class Global : public mir::wayland::ExtWorkspaceManagerV1::Global
{
public:
    Global(miral::WaylandExtensions::Context const* context) { /****/ }
    void bind(wl_resource* new_ext_workspace_manager_v1) override
    {
        new ExtWorkspaceManagerV1(new_ext_workspace_manager_v1);
    }

    /// ...
};

/// This is the actual object created on a per-client basis.
class ExtWorkspaceManagerV1 : public mir::wayland::ExtWorkspaceManagerV1
{
public:
    explicit ExtWorkspaceManagerV1(wl_resource* new_ext_workspace_manager_v1)  { /****/ }
    void commit() override  { /****/ }
    void stop() override  { /****/ }
     /// ...
};


/// This is the builder function for our protocol Global.
auto miriway::build_ext_workspace_v1_global() -> miral::WaylandExtensions::Builder
{
    return miral::WaylandExtensions::Builder
    {
        .name = ExtWorkspaceManagerV1::interface_name,
        .build = [&](auto* context) { return std::make_shared<Global>(context, wltools); }
    };
}

/// ...
```

With that, our custom Wayland protocol is integrated into our compositor and ready to use.
The next step would be to implement the method bodies of each interface in our protocol,
but we will leave that as an exercise for the reader as each implementation is highly
dependent on the compositor. As an example of how `ext-workspace-v1` could be implemented,
check out the Miriway implementation in this [pull request](https://github.com/Miriway/Miriway/pull/160).
