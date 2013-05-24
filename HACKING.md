Mir hacking guide
=================

Getting Mir
-----------

If you're reading this file then you've probably solved this one. ;)

However, for completeness Mir is a project on LaunchPad (https://launchpad.net/mir)
to grab a copy use the command:

    $ bzr branch lp:mir


Getting dependencies
--------------------
To succesfully build Mir there are a few packages required:

    $ apt-get install devscripts equivs cmake
    $ mk-build-deps --install --tool "apt-get -y" --build-dep debian/control


Building Mir
-----------

Mir is built using cmake. There are other options, but here's one way to
build the system:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make -j 8
    $ ctest


Coding Mir
----------

There's a coding style guide in the guides subdirectory. To build it into an
html file:

    $ make guides

(Or if you're reading the web version [here](cppguide/index.html)).


Code structure
--------------

Code structure: include

The include subdirectory contains header files "published" by corresponding parts
of the system. For example, include/mir/option/option.h provides a system-wide interface
for accessing runtime options published by the options component.

In many cases, there will be interfaces defined that are used by the component
and implemented elsewhere. E.g. the compositor uses RenderView which is implemented
by the surfaces component.

Files under the include directory should contain minimal implementation detail: interfaces
should not expose platform or implementation technology types etc. (And as public methods
are normally implementations of interfaces they do not use these types.)


Code structure: src

This comprises the implementation of Mir. Header files for use within the component
should be put here. The only headers from the source tree that should be included are
ones from the current component (ones that do not require a path component).


Code structure: test

This contains unit, integration and acceptance tests written using gtest/gmock. Tests
largely depend upon the public interfaces of components - but tests of units within
a component will include headers from within the source tree.


Code structure: 3rd_party

Third party code imported into our source tree for use in Mir. We try not to change
anything to avoid maintaining a fork.


Error handling strategy
-----------------------

If a function cannot meet its post-conditions it throws an exception and meets
AT LEAST the basic exception safety guarantee. It is a good idea to document the
strong and no-throw guarantees. http://www.boost.org/community/exception_safety.html


Running Mir
-----------

There are some brief guides describing how to run the Mir binaries once you have
them built. You might think it's obvious but there are some important things
you need to know to get it working, and also to prevent your existing X server
from dying at the same time.

 - \ref using_mir_on_pc
 - \ref using_mir_on_android

LTTng support
-------------

Mir provides LTTng tracepoints for various interesting events. You can enable
LTTng tracing for a Mir component by using the corresponding command-line
option or environment variable (see `--help` for more):

    $ lttng create mirsession -o /tmp/mirsession
    $ lttng enable-event -u -a
    $ lttng start
    $ mir_demo_server --msg-processor-report=lttng
    $ lttng stop
    $ babeltrace /tmp/mirsession/<trace-subdir>

LTTng-UST versions up to and including 2.1.2, and up to and including 2.2-rc2
contain a bug (lttng #538) that prevents event recording if the tracepoint
provider is dlopen()-ed at runtime, like in the case of Mir. If you have a
version of LTTng affected by this bug, you need to run:

    $ LD_PRELOAD=libmirserverlttng.so mir_demo_server --msg-processor-report=lttng

Documentation
-------------

There are design notes and an architecture diagram (.dia) in the design
subdirectory.


