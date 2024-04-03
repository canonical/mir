---
discourse: 8484
---

# OK, so what is this Wayland thing anyway?

**Or: things are complicated. Let's see if one more explanation will incrementally reduce the Internet's Wrongness™**

We've recently (ok, recently-ish) released [Mir 1.0](https://github.com/MirServer/mir/releases/tag/v1.0.0) with usable Wayland support. Yay!

That brought a bunch of publicity, including on [LWN](https://lwn.net/Articles/766178/). Some of the comments there and elsewhere betray a misunderstanding about what Wayland *is* (and is not), and this still occasionally comes up in ``#wayland``, so I'll dust off an old blog post, polish up the rusty bits, and see if I can make this clearer for people again! 

I'll give a run-down as to what the various projects in this space are and aim to do, throwing in X11 as a reference point.

## Wayland, Mir, and X - different projects, with different goals

### X11, and X.org

Everyone's familiar with their friendly neighbourhood X server. This is what we've currently got as the default desktop Linux display server. For the purposes of this blog post, X consists of:

#### The X11 protocol

You're all familiar with the X11 protocol, right? This gentle beast specifies how to talk to an X server - both the binary format of the messages you'll send and receive, and what you can expect the server to do with any given message (the semantics). There are also plenty of protocol extensions; new messages to make the server do new things, like handle more than one monitor in a non-stupid way.

#### The X11 client libraries

No-one actually fiddles with X by sending raw binary data down a socket; they use the client libraries - the modern XCB, or the boring old Xlib (also known as ``libX11.so.6``). They do the boring work of throwing binary data at the server and present the client with a more civilised view of the X server, one where you can just ``XOpenDisplay(NULL)`` and start doing stuff.

Actually, the above is a bit of a lie. Almost all the time people don't even use XCB or Xlib, they use toolkits such as GTK+ or Qt, and *they* use XLib or XCB.

#### The Xorg server

This would be the bit most obviously associated with X - the one, the only, the X.org X server! This is the actual ``/usr/bin/X`` display server we all know and love. Although there are other implementations of X11, this is all you'll ever see on the free desktop. Or on OS X, for that matter.

So that's our baseline stack - a protocol, one or more client libraries, a display server implementation. How about Wayland and Mir?

### Wayland


#### The Wayland protocol

The Wayland protocol is, like the X11 protocol, a definition of the binary data you can expect to send and receive over a Wayland socket, and the semantics associated with those binary bits. This is handled a bit differently to X11; the protocol is specified in XML, which is processed by a scanner and turned into C code. There is a binary protocol, and you can *technically* implement that protocol without using the ``wayland-scanner``-generated code, but since the Wayland EGL platform is defined in terms of the ``wl_display*`` structure from ``libwayland`` your implementation will need to be ABI compatible with the existing implementation in order for 3D accelerated clients to work.

Also different from X11 is that everything's treated as an extension - you deal with all the interfaces in the core protocol the same way as you deal with any extensions you create. And you create a lot of extensions - for example, the core protocol doesn't have any buffer-passing mechanism other than SHM, so there's an extension for drm buffers in Mesa. Likewise, the NVIDIA drivers define their own internal protocol and a protocol for the compositor to support. The Weston reference compositor also has a bunch of extensions, both for ad-hoc things like the compositor<->desktop-shell interface, and for things like XWayland.

#### The Wayland client library

Or ``libwayland``. A bit like XCB and Xlib, this is basically just an IPC library. Unlike XCB and Xlib it can also used by a Wayland server for server→client communication. Also unlike XCB and Xlib, it's programmatically generated from the protocol definition. It's quite a nice IPC library, really. Like XCB and Xlib, you're not really expected to use this, anyway; you're expected to use a toolkit like Qt or GTK+, and EGL + your choice of Khronos drawing API if you want to be funky.
There's also a library for reading X cursor themes in there.

#### The Wayland server?

This is where it diverges; there *is no* Wayland server in the sense that there's an Xorg server. There's Weston, the reference compositor, which is growing ``libweston`` to do some custom shell work and is used in various places. There's ``mutter``, the GNOME window manager which has grown a Wayland server implementation. Likewise, there's ``kwin``, KDE's window manager that grew a Wayland server implementation. There's also a swathe of lesser-known display-server-compositor-window-manager projects, like [sway](https://swaywm.org/).

Of course, there's also now anything based on Mir ☺. [Unity8](https://unity8.io/) and [EGMDE](https://github.com/AlanGriffiths/egmde) are both now Wayland servers!

Desktop environments are expected to write their own Wayland server, using the protocol and client libraries, and they have.

### Mir

And so we get to Mir.

Where the Wayland libraries are all about IPC, Mir is about producing a library to do the drudge work of a display-server-compositor-window-manager-thing, so in this way it's more like Xorg than Wayland.

Mir *does* still have a (not-guaranteed-stable) protocol and client library in ``libmirclient`` - the Wayland server implementation is an *additional* client frontend to Mir's core - but ``libmirclient`` is deprecated and we will not be pursuing further development of it. Supporting the standard desktop Wayland window management extensions means much less work for the Mir team - we only need to focus on the Mir code itself, not *also* on an EGL platform + associated Mesa patches and GTK+ patches and Qt patches and SDL patches and…
