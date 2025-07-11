# Mir hacking guide

## Building & running

For build instructions, and a brief guide describing how to run the *Mir binaries* please see:
- *[Getting and Using Mir](./doc/sphinx/tutorial/getting-started-with-mir.md)*.
_You might think it's obvious but there are some important things you need to know to get it working,
and also to prevent your existing *X server* from dying at the same time!_


## Style guide

There is a *coding style guide* in the guides subdirectory. To build it into an
*html* file:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make guides

(Or online, [here](https://canonical-mir.readthedocs-hosted.com/latest/_static/cppguide)).


## Code structure

_**include/**_

The include subdirectory contains header files "published" by corresponding parts
of the system. For example, *include/mir/option/option.h* provides a system-wide interface
for accessing runtime options published by the options component.

In many cases, there will be interfaces defined that are used by the component
and implemented elsewhere. *E.g. the compositor uses RenderView which is implemented
by the surfaces component.*

Files under the include directory should contain minimal implementation detail: interfaces
should not expose platform or implementation technology types *etc.* (And as public methods
are normally implementations of interfaces they do not use these types.)


_**src/**_

This comprises the implementation of *Mir*. Header files for use within the component
should be put here. The only headers from the source tree that should be included are
ones from the current component (ones that do not require a path component).


_**test/**_

This contains unit, integration and acceptance tests written using *gtest/gmock*. Tests
largely depend upon the public interfaces of components - but tests of units within
a component will include headers from within the source tree.


## Error handling strategy

If a function cannot meet its post-conditions it throws an exception and meets
__AT LEAST__ the basic exception safety guarantee. It is a good idea to document the
strong and no-throw guarantees._http://www.boost.org/community/exception_safety.html_

A function is not required to check its preconditions (there should be no
tests that preconditions failures are reported). This means that 
preconditions may be verified using the "*assert"* macro - which may or may
not report problems (depending upon the __NDEBUG__ define).


## Implicit rules

There are a lot of __pointers__ (mostly smart, but a few raw ones) passed
around in the code. We have adopted the general rule that pointers are
expected to refer to valid objects. This avoids repetitive tests for
validity. Unless otherwise documented functions and constructors that
take pointer parameters have validity of the referenced objects as a
precondition. Exceptions to the rule must be of limited scope and 
documented.


## Documentation

There are *design notes* and an *architecture diagram* (.dia) in the design
subdirectory.

## Debugging
You can configure *Mir* to provide runtime information helpful for debugging
by enabling component reports:

- *[Component Reports](./doc/sphinx/explanation/component_reports.md)*

## Speeding up compilation

Since *Mir* is a large C++ project it can take a long time to compile and make developing features slow to test.
You can speed this up using [ccache](https://ccache.dev/) to cache compilation,
disable precompiled headers (helps caching) and choose to use a faster linker like [mold](https://github.com/rui314/mold):

    $ sudo apt install ccache mold
    $ cd build
    $ cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DMIR_USE_PRECOMPILED_HEADERS=OFF -DMIR_USE_LD=mold ..

## Mesa/GBM debugging

*Mir* interacts heavily with GL, GBM, and KMS, and sometimes debugging is
easier if you can trace calls down into those layers.

The GL library is particularly performance-sensitive, so the *Mesa*
implementation is aggressively optimised; a debugging build of *Mesa* can
be signficantly easier to work with.

To make such a debug build,

    $ . /etc/os-release                   # Get release information
    $ cd $A_TEMPORARY_DIRECTORY           # e.g. ~/mesa-build
    $ apt source mesa                     # Pull the mesa source package
    $ cd mesa-$UPSTREAM_VERSION           # e.g. mesa-25.0.3
    $ <edit debian/rules; find the line -Db_ndebug=true and change it to =false>
    $ DEB_BUILD_OPTIONS="noopt nostrip" sbuild -d $UBUNTU_CODENAME -j 32
    $ sudo dpkg --install --force-overwrite ../*.deb

The important components here are:
1. editing the `debian/rules` file to disable
   `ndebug`; when this is enabled *Mesa* disables various codepaths useful in
   debugging.
2. Building with the `noopt` and `nostrip` `DEB_BUILD_OPTIONS`; this turns off
   optimisation (making stacks easier to debug) and turns off stripping the
   debug symbols from the binaries.
3. `--force-overwrite` in the `dpkg --install` command; this is needed because
   you will almost certainly have the `:i386` versions of mesa packages installed
   as well, and these share some files that `dpkg` requires match exactly,
   which will not happen for your local build.

Due to a quirk in the *Mesa* build process, getting `gdb` to find the
corresponding source requires manually specifying a directory to find
them in. Following these instructions,
`directory $A_TEMPORARY_DIRECTORY/mesa-$UPSTREAM_VERSION/build`
will direct `gdb` to find the source files correctly.
