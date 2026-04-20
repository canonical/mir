# XDG Portals support

Mir does not have built-in support for XDG portals, but it is possible to use 
them with Mir-based compositors. This document provides an overview of how to
use XDG portals with Mir.

## What are XDG portals?

XDG portals are a set of D-Bus APIs that allow applications to access certain 
system resources in a secure and sandboxed way. They are typically used in
desktop environments to provide access to sensitive functionality like file 
access, opening URIs, screen capture and more.

## The role of the compositor in XDG portals

The portal backend is responsible for implementing the D-Bus APIs that 
applications use to access the portals. This backend can be implemented in a 
variety of ways, but it typically runs as a separate process that communicates
with the compositor and other system components to provide the necessary 
functionality.

A list of XDK portals and the functions they provide can be found in the
[XDG portal documentation](https://flatpak.github.io/xdg-desktop-portal/docs/api-reference.html).

From the perspective of a compositor such as Mir, the portal backend is just 
another client that it needs to support. The compositor needs to provide the 
necessary support for the portal backend to function correctly, such as 
allowing it to access the necessary Wayland protocol extensions and providing it
with the necessary permissions. But a lot of the functionality provided does not
require any support from the compositor.

## The portal backends and Mir

Various portal backends exist, such as `xdg-desktop-portal-wlr` and 
`xdg-desktop-portal-cosmic`. These backends may be tied to a specific 
compositor, but many are designed to work with a variety of compositors, 
including Mir-based compositors. They typically use standard Wayland protocols 
and extensions to communicate with the compositor, so they should work with any
compositor that supports those protocols.

There are Wayland protocol extensions that are needed for the portal backend to
function correctly. Mir has, or plans, support for these protocols so the portal
backend can function correctly with Mir-based compositors.

## Permissions and security considerations

Because the Wayland extensions used by the portal backends are privileged Mir
does not, by default, enable them for all applications. The portal backend will
need to be granted the necessary permissions to use them.

This should be done carefully, as granting too many permissions to the portal
backend or allowing too many processes access could potentially allow access to
sensitive resources or perform actions that could compromise the security of the
system. It is important to only grant the necessary permissions and to ensure 
that the portal backend is properly sandboxed.

The exact steps for doing this will depend on the specific compositor and portal 
backend being used.

For one example, see [Enable screencasting](how-to-enable-screencasting).