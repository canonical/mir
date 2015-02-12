A brief guide for versioning symbols in the Mir DSOs {#dso_versioning_guide}
====================================================

So, what do I have to do?
-------------------------

There are more detailed descriptions below, but as a general rule:

 - If you add a new symbol, add it to a _new_ dotted-decimal version stanza,
   like `MIR_CLIENT_8.1`, `MIR_CLIENT_8.2`, etc.
 - If you change the behaviour or signature of a symbol and not change SONAME,
   see "Change symbols without breaking ABI" below
 - If you change SONAME, collect all previous symbol version stanzas into a
   single labelled with the new SOVER. For example, remove the `MIR_CLIENT_8`,
   `MIR_CLIENT_8.1`, and `MIR_CLIENT_8.2` stanzas and consolidate their
   contents into a single new `MIR_CLIENT_9` stanza.

Can I have some details?
------------------------

Sure.

Mir is a set of libraries, one C++ library for writing display-
server/compositor/shells and one C library for writing clients (or, more
usually, toolkits for clients) that use a Mir display-server for output. Mir
also has internal dynamic libraries for platform support - drivers - and may in
future allow the same with extensions to the core functionality. As such, the
ABI of these interfaces is important to keep in mind.

Mir uses the ELF symbol versioning support. This provides three advantages:

 - Consumers of the Mir libraries can know at load time rather than symbol
   resolution time whether the library exposes all the symbols they expect.
 - We can drop or change the behaviour of symbols without breaking ABI by
   exposing multiple different implementations under different versions, and
 - We can (modulo protobuf singletons in our current implementation, and with
   some care) safely load multiple different versions of Mir libraries into the
   same process.

When should I bump SONAME?
--------------------------

There are varying standards for when to bump SONAME. In Mir we choose to bump
the SONAME of a library whenever we make a change that could cause a binary
linked to the library to fail _as long as_ the binary is using only public
interfaces and (where applicable) relying on documented behaviour. In general,
changes that make an interface work as described by its documentation will not
result in SONAME bumps.

With that explanation, you _should_ bump SONAME when:

 - You remove a public symbol from a library
 - You change the signature of a public symbol _without_ retaining the previous
   signature exposed under the old versioning.
 - You change the behaviour of a public symbol _without_ retaining the previous
   behaviour exposed with the old versioning.

If you are changing the behaviour of an interface, think about whether it's easy
to maintain the old interface in parallel. If it is, you should consider
providing both under different versions. This should become easier over time as
the Mir ABI becomes more stable and also more valuable over time as the Mir
libraries become more widely used.

Load-time version detection
---------------------------

When using versioned symbols the linker adds an extra, special symbol containing
the version(s) exported from the library. Consumers of the library resolve this
on library load. For example:

    $ objdump -C -T lib/libmirclient.so
    …
    00000000002a2080  w   DO .data.rel.ro   0000000000000080  MIR_CLIENT_8 vtable for mir::client::DefaultConnectionConfiguration
    0000000000000000 g    DO *ABS*  0000000000000000  MIR_CLIENT_8 MIR_CLIENT_8
    0000000000030ed2 g    DF .text  0000000000000098  MIR_CLIENT_8 mir::client::DefaultConnectionConfiguration::the_rpc_report()
    …

This shows the special `MIR_CLIENT_8` symbol of the current libmirclient, along
with a versioned symbol in the read-only data segment (the vtable for
`mir::client::DefaultConnectionConfiguration`) and a versioned symbol in the
text segment (the implementation of
`mir::client::DefaultConnectionConfiguration::the_rpc_report()`). If a client
needed a symbol versioned with `MIR_CLIENT_9`, it would try to resolve this at
load time and fail, rather than failing when the symbol was first referenced -
possibly much later, and more confusingly.

### So what do I have to do to make this work?

When you add new symbols, add them to a new `version` block in the relevant
`symbols.map` file, like so:

    MIR_CLIENT_8 {
        global:
            mir_connect_sync;
            ...
            /* Other symbols go here */
    };

    MIR_CLIENT_8.1 {
        global:
            mir_connect_new_symbol;
        local:
            *;
    } MIR_CLIENT_8;

Note that the script is read top to bottom; wildcards are greedily bound when
first encountered, so to avoid surprises you should only have a wildcard in the
final stanza.

Change symbols without breaking ABI
-----------------------------------

ELF DSOs can have multiple implementations for the same symbol with different
versions. This means that you can change the signature or behaviour of a symbol
without breaking dependants that use the old behaviour. While there can be as
many different implementations with different versions as you want, there can
only be one default implementation - this is what the linker will resolve to
when building a dependant project.

Binding different implementations to the versioned symbol is done with `__asm__`
directives in the relevant source file(s). The default implementation is
specified with `symbol_name@@VERSION`; other versions are specified with
`symbol_name@VERSION`.

Note that this does _not_ require a change in SONAME. Binaries that have been
linked against the old library will continue to work and resolve to the old
implementation. Binaries linked against the new library will resolve to the new
(default) implementation.

### So, what do I have to do to make this work?
For example, if you wanted to change the signature of
`mir_connection_create_surface` to take a new parameter:

`mir_connection_api.cpp`:

    __asm__(".symver old_mir_connection_create_surface,mir_connection_create_surface@MIR_CLIENT_8");

    extern "C" MirWaitHandle* old_mir_connection_create_surface(...)
    /* The old implementation */

    /* The @@ specifies that this is the default version */
    __asm__(".symver mir_connection_create_surface,mir_connection_create_surface@@@MIR_CLIENT_8.1");
    MirWaitHandle* mir_connection_create_surface(...)
    /* The new implementation */

`symbols.map`:

    MIR_CLIENT_8 {
        global:
            ...
            mir_connection_create_surface;
            ...
    };

    MIR_CLIENT_8.1 {
        global:
            ...
            mir_connection_create_surface;
            ...
        local:
            *;
    } MIR_CLIENT_8;

Safely load multiple versions of a library into the same address space
----------------------------------------------------------------------

This benefit is currently theoretical, as there seems to be a Protobuf singleton
that aborts if we try this. But should that be resolved, it's theoretically
possible and of some benefit...

This situation will come about - the Qtmir plugin links to libmirclient and also
libEGL, and libEGL will link to libmirclient itself. There is no guarantee that
Qtmir and libEGL will link to the same SONAME, and so a process can end up
trying to load both `libmirclient.so.8` and `libmirclient.so.9` into its address
space. Without symbol versioning this is potentially broken - there's no
mechanism for libEGL to only resolve symbols from `libmirclient.so.8` and Qtmir
to only resolve symbols from `libmirclient.so.9`, so in cases where symbols have
changed use of those symbols will break.

By versioning the symbols we ensure that code always gets exactly the symbol
implementation it expects, even when multiple library versions are loaded.

### So, what do I have to do to make this work?

Ensure that different implementations of a symbol have different versions.

Additionally there's the complication of passing objects between different
versions. For the moment, we can not bother trying to make this work.


See also: 
---------
[Binutils manual](https://sourceware.org/binutils/docs/ld/VERSION.html)

[Former glibc maintainer's DSO guide](http://www.akkadia.org/drepper/dsohowto.pdf)
