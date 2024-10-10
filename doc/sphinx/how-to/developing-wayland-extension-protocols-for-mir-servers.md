---
discourse: 11993
---

# Developing Wayland extension protocols for Mir servers

With the [Mir 1.3 release](https://discourse.ubuntu.com/t/mir-1-3-0-release/11638/2) we  smoothed the way for shells adding Wayland extension protocols in Mir based shells.

## The worked example

To create a "worked example" the first thing I needed was a downstream shell that plausibly needed an extension writing. For obvious reasons I chose [egmde](https://discourse.ubuntu.com/t/egmde-a-project-that-uses-mir/7129) as the example server. For the extension protocol I chose ["primary selection"](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/unstable/primary-selection/primary-selection-unstable-v1.xml). While a protocol that allows every application to "spy" on the clipboard may not meet the security criteria we use for Mir in IoT, for a desktop environment like egmde it is a great convenience.

The changes to egmde can be seen in [this pull request](https://github.com/AlanGriffiths/egmde/pull/18). I'll discuss these in more detail below, but for those that want to follow along the following setup is needed to build the code.

The following Mir packages are needed for building egmde and for developing the Wayland extension:
```bash
sudo add-apt-repository --update ppa:mir-team/release
sudo apt install libmiral-dev mir-graphics-drivers-desktop
sudo apt install libmirwayland-dev mirtest-dev wlcs
```
The `libmirwayland-dev` provides the tooling needed to implement the protocol in Mir while `mirtest-dev` and `wlcs` provide the testing framework.

## wlcs tests for primary selection

While working on this I proposed adding some ["primary selection" tests](https://github.com/canonical/wlcs/pull/106) to wlcs and they are included in the 1.1 release of wlcs. Here's an example of what the tests look like:

```cpp
TEST_F(PrimarySelection, source_sees_request)
{
    MockPrimarySelectionSourceListener  source_listener{source_app.source};
    PrimarySelectionOfferListener       offer_listener;
    StubPrimarySelectionDeviceListener  device_listener{sink_app.device, offer_listener};
    source_app.offer(any_mime_type);
    source_app.set_selection();
    sink_app.roundtrip();
    ASSERT_THAT(device_listener.selected, NotNull());

    EXPECT_CALL(source_listener, send(_, _, _))
        .Times(1)
        .WillRepeatedly(Invoke([&](auto*, auto*, int fd) { close(fd); }));

    Pipe pipe;
    zwp_primary_selection_offer_v1_receive(device_listener.selected, any_mime_type, pipe.source);
    sink_app.roundtrip();
    source_app.roundtrip();
}
```

## Implementing a protocol extension

The first step in implementing a protocol extension is to use the protocol generator from the `libmirwayland-dev` package, I've added two directories to egmde: `wayland-protocols` and `wayland-generated`. The first of these contains a copy of the protocol definition, the second the generated files and the `cmake` script to generate them. Only the latter is worth reproducing here:
```cmake
set(PROTOCOL "primary-selection-unstable-v1")
set(PROTOCOL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../wayland-protocols/${PROTOCOL}.xml")
set(GENERATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/primary-selection-unstable-v1_wrapper")

add_custom_command(
    OUTPUT ${GENERATE_FILE}.cpp
    OUTPUT ${GENERATE_FILE}.h
    DEPENDS ${PROTOCOL_FILE}
    COMMAND "sh" "-c" "mir_wayland_generator zwp_ ${PROTOCOL_FILE} header >${GENERATE_FILE}.h"
    COMMAND "sh" "-c" "mir_wayland_generator zwp_ ${PROTOCOL_FILE} source >${GENERATE_FILE}.cpp"
)

add_custom_target(primary-selection-unstable
    DEPENDS ${GENERATE_FILE}.cpp
    DEPENDS ${GENERATE_FILE}.h
    SOURCES
        ${GENERATE_FILE}.cpp
        ${GENERATE_FILE}.h
)
```
The generator cannot implement the semantics of the protocol: it just provides the "hooks" into which to write the code. The latter can be found in a couple of files `egprimary_selection_device_controller.cpp` and `primary_selection.cpp` (this is only split this way as a lot of logic can be shared with `gtk_primary_selection`.)

All these generated and and-written files are combined into a static library that is used both by egmde and by a new module `egmde-wlcs.so` that provides the wlcs test fixture for egmde.

## wlcs test fixture

To run wlcs tests requires a "test fixture" that allows the test executable to load and control the compositor. Mir makes it very easy to implement a fixture: here is the entire "fixture" code for egmde:

```cpp
#include "primary_selection.h"
#include "gtk_primary_selection.h"

#include <miral/wayland_extensions.h>
#include <miral/test_wlcs_display_server.h>

namespace
{
struct TestWlcsDisplayServer : miral::TestWlcsDisplayServer
{
    miral::WaylandExtensions wayland_extensions;

    TestWlcsDisplayServer(int argc, char const** argv) :
        miral::TestWlcsDisplayServer{argc, argv}
    {
        wayland_extensions.add_extension(egmde::primary_selection_extension());
        wayland_extensions.add_extension(egmde::gtk_primary_selection_extension());
        add_server_init(wayland_extensions);
    }
};

WlcsExtensionDescriptor const extensions[] = {
    {primary_selection_extension.name.c_str(), 1},
    {gtk_primary_selection_extension.name.c_str(), 1},
};

WlcsIntegrationDescriptor const descriptor{
    1,
    sizeof(extensions) / sizeof(extensions[0]),
    extensions
};

WlcsIntegrationDescriptor const* get_descriptor(WlcsDisplayServer const* /*server*/)
{
    return &descriptor;
}

WlcsDisplayServer* wlcs_create_server(int argc, char const** argv)
{
    auto server = new TestWlcsDisplayServer(argc, argv);

    server->get_descriptor = &get_descriptor;
    return server;
}

void wlcs_destroy_server(WlcsDisplayServer* server)
{
    delete static_cast<TestWlcsDisplayServer*>(server);
}
}

extern WlcsServerIntegration const wlcs_server_integration {
    1,
    &wlcs_create_server,
    &wlcs_destroy_server,
};
```

## Running the tests

With all this in place, we just need to run our updated wlcs with the egmde test fixture:

```text
$ `pkg-config --variable test_runner wlcs` cmake-build-debug/egmde-wlcs.so --gtest_filter=Prim*
Note: Google Test filter = Prim*
[==========] Running 5 tests from 1 test case.
[----------] Global test environment set-up.
[----------] 5 tests from PrimarySelection
[ RUN      ] PrimarySelection.source_can_offer
[       OK ] PrimarySelection.source_can_offer (9 ms)
[ RUN      ] PrimarySelection.sink_can_listen
[       OK ] PrimarySelection.sink_can_listen (8 ms)
[ RUN      ] PrimarySelection.sink_can_request
[       OK ] PrimarySelection.sink_can_request (11 ms)
[ RUN      ] PrimarySelection.source_sees_request
[       OK ] PrimarySelection.source_sees_request (9 ms)
[ RUN      ] PrimarySelection.source_can_supply_request
[       OK ] PrimarySelection.source_can_supply_request (8 ms)
[----------] 5 tests from PrimarySelection (45 ms total)

[----------] Global test environment tear-down
[==========] 5 tests from 1 test cases run. (45ms total elapsed)
[  PASSED  ] 5 tests
```

## The real world

I've not (yet) found a client toolkit that uses "zwp_primary_selection_device_manager_v1", so I've yet to check whether this works with real applications. But the principle steps of this article remain valid even if there are errors in the implementation of this protocol.
