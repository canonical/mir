# Force out of source build
%undefine __cmake_in_source_build

# Use ccache
%bcond ccache 0

# Use clang
%bcond clang 0

# Use lld
%bcond lld 0

# Use mold
%bcond mold 0

%if %{with clang}
# Force clang toolchain
%global toolchain clang
# Disable LTO with clang
%global _lto_cflags %{nil}
%endif

# Debug build with extra compile time checks
%bcond debug 0

# Run tests by default
%bcond run_tests 1

# Track various library soversions
%global miral_sover 7
%global mircommon_sover 11
%global mircore_sover 2
%global miroil_sover 8
%global mirplatform_sover 33
%global mirserver_sover 66
%global mirwayland_sover 5
%global mirplatformgraphics_sover 23
%global mirplatforminput_sover 10

Name:           mir
Version:        2.22.0
Release:        0%{?dist}
Summary:        Next generation Wayland display server toolkit

# mircommon is LGPL-2.1-only/LGPL-3.0-only, everything else is GPL-2.0-only/GPL-3.0-only
License:        (GPL-2.0-only or GPL-3.0-only) and (LGPL-2.1-only or LGPL-3.0-only)
URL:            https://canonical.com/mir
Source0:        https://github.com/canonical/%{name}/releases/download/v%{version}/%{name}-%{version}.tar.xz

%if %{with ccache}
BuildRequires:  ccache
%endif
%if %{with clang}
BuildRequires:  clang
%else
BuildRequires:  gcc-c++
%endif
%if %{with lld}
BuildRequires:  lld
%endif
%if %{with mold}
BuildRequires:  mold
%endif
BuildRequires:  cmake, ninja-build, diffutils, doxygen, graphviz, lcov, gcovr
BuildRequires:  /usr/bin/xsltproc
BuildRequires:  boost-devel
BuildRequires:  python3
BuildRequires:  glm-devel
BuildRequires:  glog-devel, lttng-ust-devel, systemtap-sdt-devel
BuildRequires:  gflags-devel
BuildRequires:  python3-pillow

# Everything detected via pkgconfig
BuildRequires:  pkgconfig(egl)
BuildRequires:  pkgconfig(epoxy)
BuildRequires:  pkgconfig(freetype2)
BuildRequires:  pkgconfig(gbm) >= 9.0.0
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gmock) >= 1.8.0
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(gio-unix-2.0)
BuildRequires:  pkgconfig(gtest) >= 1.8.0
BuildRequires:  pkgconfig(libdisplay-info)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(libevdev)
BuildRequires:  pkgconfig(libinput)
BuildRequires:  pkgconfig(libudev)
BuildRequires:  pkgconfig(libxml++-2.6)
BuildRequires:  pkgconfig(nettle)
BuildRequires:  pkgconfig(umockdev-1.0) >= 0.6
BuildRequires:  pkgconfig(uuid)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(xcb)
BuildRequires:  pkgconfig(xcb-composite)
BuildRequires:  pkgconfig(xcb-xfixes)
BuildRequires:  pkgconfig(xcb-render)
BuildRequires:  pkgconfig(xcursor)
BuildRequires:  pkgconfig(xkbcommon)
BuildRequires:  pkgconfig(xkbcommon-x11)
BuildRequires:  pkgconfig(yaml-cpp)
BuildRequires:  pkgconfig(wlcs)

# pkgconfig(egl) is now from glvnd, so we need to manually pull this in for the Mesa specific bits...
BuildRequires:  mesa-libEGL-devel

# For some reason, this doesn't get pulled in automatically into the buildroot
BuildRequires:  libatomic

# For detecting the font for CMake
BuildRequires:  gnu-free-sans-fonts

# For validating the desktop file for mir-demos
BuildRequires:  %{_bindir}/desktop-file-validate

# For the tests
BuildRequires:  dbus-daemon
BuildRequires:  python3-dbusmock
BuildRequires:  xorg-x11-server-Xwayland

# Add architectures as verified to work
%ifarch %{ix86} x86_64 %{arm} aarch64
BuildRequires:  valgrind
%endif


%description
Mir is a Wayland display server toolkit for Linux systems,
with a focus on efficiency, robust operation,
and a well-defined driver model.

%package devel
Summary:       Development files for Mir
Requires:      %{name}-common-libs%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}
Requires:      %{name}-server-libs%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}
Requires:      %{name}-lomiri-libs%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}
Requires:      %{name}-test-libs-static%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}
# Documentation can no longer be built properly
Obsoletes:     %{name}-doc < 2.15.0

%description devel
This package provides the development files to create compositors
built on Mir.

%package internal-devel
Summary:       Development files for Mir exposing private internals
Requires:      %{name}-devel%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}

%description internal-devel
This package provides extra development files to create compositors
built on Mir that need access to private internal interfaces.

%package common-libs
Summary:       Common libraries for Mir
License:       LGPL-2.1-only or LGPL-3.0-only
# mirclient is gone...
Obsoletes:     %{name}-client-libs < 2.6.0
# debug extension for mirclient is gone...
Obsoletes:     %{name}-client-libs-debugext < 1.6.0
# mir utils are gone...
Obsoletes:     %{name}-utils < 2.0.0
# Ensure older mirclient doesn't mix in
Conflicts:     %{name}-client-libs < 2.6.0

%description common-libs
This package provides the libraries common to be used
by Mir clients or Mir servers.

%package lomiri-libs
Summary:       Lomiri compatibility libraries for Mir
License:       GPL-2.0-only or GPL-3.0-only
Requires:      %{name}-common-libs%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}
Requires:      %{name}-server-libs%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}

%description lomiri-libs
This package provides the libraries for Lomiri to use Mir
as a Wayland compositor.

%package server-libs
Summary:       Server libraries for Mir
License:       GPL-2.0-only or GPL-3.0-only
Requires:      %{name}-common-libs%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}

%description server-libs
This package provides the libraries for applications
that use the Mir server.

%package test-tools
Summary:       Testing tools for Mir
License:       GPL-2.0-only or GPL-3.0-only
Requires:      %{name}-server-libs%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}
Recommends:    %{name}-demos
Recommends:    glmark2
Recommends:    xorg-x11-server-Xwayland
Requires:      wlcs
# mir-perf-framework is no more...
Obsoletes:     python3-mir-perf-framework < 2.6.0
# Ensure mir-perf-framework is not installed
Conflicts:     python3-mir-perf-framework < 2.6.0

%description test-tools
This package provides tools for testing Mir.

%package demos
Summary:       Demonstration applications using Mir
License:       GPL-2.0-only or GPL-3.0-only
Requires:      %{name}-server-libs%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}
Requires:      inotify-tools
Requires:      hicolor-icon-theme
Requires:      xorg-x11-server-Xwayland
Requires:      xkeyboard-config
# For some of the demos
Requires:      gnu-free-sans-fonts

%description demos
This package provides applications for demonstrating
the capabilities of the Mir display server.

%package test-libs-static
Summary:       Testing framework library for Mir
License:       GPL-2.0-only or GPL-3.0-only
Requires:      %{name}-devel%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}

%description test-libs-static
This package provides the static library for building
Mir unit and integration tests.


%prep
%autosetup


%conf
%cmake	-GNinja \
	%{?with_ccache:-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache} \
	%{?with_debug:-DCMAKE_BUILD_TYPE=Debug} \
	%{!?with_debug:-DMIR_FATAL_COMPILE_WARNINGS=OFF} \
	%{?with_lld:-DMIR_USE_LD=lld} \
	%{?with_mold:-DMIR_USE_LD=mold} \
	-DMIR_USE_PRECOMPILED_HEADERS=OFF \
	-DCMAKE_INSTALL_LIBEXECDIR="usr/libexec/mir" \
	-DMIR_PLATFORM="atomic-kms;gbm-kms;wayland;x11"


%build
%cmake_build


%install
%cmake_install


%check
%if %{with run_tests}
export XDG_RUNTIME_DIR=$(mktemp -d)
%ctest
rm -rf $XDG_RUNTIME_DIR
%endif
desktop-file-validate %{buildroot}%{_datadir}/applications/miral-shell.desktop


%files devel
%license COPYING.*
%{_bindir}/mir_wayland_generator
%{_libdir}/libmir*.so
%{_libdir}/pkgconfig/mir*.pc
%exclude %{_libdir}/pkgconfig/mir*internal.pc
%{_includedir}/mir*/
%exclude %{_includedir}/mir*internal/

%files internal-devel
%license COPYING.*
%{_libdir}/pkgconfig/mir*internal.pc
%{_includedir}/mir*internal/

%files common-libs
%license COPYING.LGPL*
%doc README.md
%{_libdir}/libmircore.so.%{mircore_sover}
%{_libdir}/libmircommon.so.%{mircommon_sover}
%{_libdir}/libmirplatform.so.%{mirplatform_sover}
%dir %{_libdir}/%{name}

%files lomiri-libs
%license COPYING.GPL*
%doc README.md
%{_libdir}/libmiroil.so.%{miroil_sover}

%files server-libs
%license COPYING.GPL*
%doc README.md
%{_libdir}/libmiral.so.%{miral_sover}
%{_libdir}/libmirserver.so.%{mirserver_sover}
%{_libdir}/libmirwayland.so.%{mirwayland_sover}
%dir %{_libdir}/%{name}/server-platform
%{_libdir}/%{name}/server-platform/graphics-atomic-kms.so.%{mirplatformgraphics_sover}
%{_libdir}/%{name}/server-platform/graphics-gbm-kms.so.%{mirplatformgraphics_sover}
%{_libdir}/%{name}/server-platform/graphics-wayland.so.%{mirplatformgraphics_sover}
%{_libdir}/%{name}/server-platform/input-evdev.so.%{mirplatforminput_sover}
%{_libdir}/%{name}/server-platform/renderer-egl-generic.so.%{mirplatformgraphics_sover}
%{_libdir}/%{name}/server-platform/server-virtual.so.%{mirplatformgraphics_sover}
%{_libdir}/%{name}/server-platform/server-x11.so.%{mirplatformgraphics_sover}

%files test-tools
%license COPYING.GPL*
%{_bindir}/mir-*test*
%{_bindir}/mir_*test*
%dir %{_libdir}/%{name}/tools
%{_libdir}/%{name}/tools/libmirserverlttng.so
%dir %{_libdir}/%{name}
%{_libdir}/%{name}/miral_wlcs_integration.so
%dir %{_libdir}/%{name}/server-platform
%{_libdir}/%{name}/server-platform/graphics-dummy.so.%{mirplatformgraphics_sover}
%{_libdir}/%{name}/server-platform/input-stub.so.%{mirplatforminput_sover}
%{_datadir}/%{name}/expected_wlcs_failures.list

%files test-libs-static
%license COPYING.GPL*
%{_libdir}/libmir-test-assist.a
%{_libdir}/libmir-test-assist-internal.a

%files demos
%license COPYING.GPL*
%doc README.md
%{_bindir}/mir_demo_*
%{_bindir}/mir-x11-kiosk*
%{_bindir}/miral-*
%{_datadir}/applications/miral-shell.desktop
%{_datadir}/icons/hicolor/scalable/apps/spiral-logo.svg


%changelog
* Tue Aug 26 2025 Neal Gompa <ngompa@fedoraproject.org> - 2.22.0-1
- Update to 2.22.0

* Thu Jul 24 2025 Fedora Release Engineering <releng@fedoraproject.org> - 2.21.1-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_43_Mass_Rebuild

* Thu Jul 17 2025 Neal Gompa <ngompa@fedoraproject.org> - 2.21.1-1
- Update to 2.21.1
- Start tracking library soversions properly

* Thu Jul 10 2025 Neal Gompa <ngompa@fedoraproject.org> - 2.21.0-2
- Drop EGLStreams support

* Fri Jun 27 2025 Neal Gompa <ngompa@fedoraproject.org> - 2.21.0-1
- Rebase to 2.21.0

* Wed Apr 16 2025 Shawn W. Dunn <sfalken@cloverleaf-linux.org> - 2.20.2-1
- Update to 2.20.2

* Thu Mar 20 2025 Shawn W. Dunn <sfalken@cloverleaf-linux.org> - 2.20.1-1
- Update to 2.20.1

* Wed Mar 12 2025 Shawn W. Dunn <sfalken@cloverleaf-linux.org> - 2.20.0-1
- Update to 2.20.0

* Fri Feb 21 2025 Neal Gompa <ngompa@fedoraproject.org> - 2.19.3-3
- Backport fixes for LXQt Wayland

* Wed Jan 22 2025 Benjamin A. Beasley <code@musicinmybrain.net> - 2.19.3-2
- Rebuilt for gtest 1.15.2

* Mon Jan 20 2025 Neal Gompa <ngompa@fedoraproject.org> - 2.19.3-1
- Update to 2.19.3

* Fri Jan 17 2025 Fedora Release Engineering <releng@fedoraproject.org> - 2.19.2-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_42_Mass_Rebuild

* Tue Dec 03 2024 Neal Gompa <ngompa@fedoraproject.org> - 2.19.2-1
- Update to 2.19.2

* Mon Dec 02 2024 Neal Gompa <ngompa@fedoraproject.org> - 2.19.0-1
- Update to 2.19.0

* Fri Oct 25 2024 Orion Poplawski <orion@nwra.com> - 2.18.2-2
- Rebuild for yaml-cpp 0.8

* Fri Sep 27 2024 Neal Gompa <ngompa@fedoraproject.org> - 2.18.2-1
- Update to 2.18.2

* Thu Sep 26 2024 Neal Gompa <ngompa@fedoraproject.org> - 2.18.0-1
- Update to 2.18.0

* Thu Jul 18 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.17.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_41_Mass_Rebuild

* Wed May 15 2024 Neal Gompa <ngompa@fedoraproject.org> - 2.17.0-1
- Update to 2.17.0

* Wed Apr 10 2024 Neal Gompa <ngompa@fedoraproject.org> - 2.16.4-1
- Update to 2.16.4

* Tue Feb 27 2024 Neal Gompa <ngompa@fedoraproject.org> - 2.16.3-1
- Update to 2.16.3

* Thu Jan 25 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.15.0-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_40_Mass_Rebuild

* Sun Jan 21 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.15.0-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_40_Mass_Rebuild

* Wed Jan 17 2024 Jonathan Wakely <jwakely@redhat.com> - 2.15.0-3
- Rebuilt for Boost 1.83
- Add patch for missing headers needed with GCC 14

* Tue Oct 31 2023 Terje Rosten <terje.rosten@ntnu.no> - 2.15.0-2
- Rebuild for gtest 1.14.0

* Fri Sep 08 2023 Neal Gompa <ngompa@fedoraproject.org> - 2.15.0-1
- Update to 2.15.0

* Thu Jul 20 2023 Fedora Release Engineering <releng@fedoraproject.org> - 2.12.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_39_Mass_Rebuild

* Mon Feb 20 2023 Jonathan Wakely <jwakely@redhat.com> - 2.12.0-2
- Rebuilt for Boost 1.81

* Wed Feb 01 2023 Neal Gompa <ngompa@fedoraproject.org> - 2.12.0-1
- Update to 2.12.0
- Convert license identifiers to SPDX notation

* Tue Jan 24 2023 Benjamin A. Beasley <code@musicinmybrain.net> - 2.8.0-7
- Rebuilt for gtest 1.13.0 (close RHBZ#2163843)
- Patch for GCC 13

* Thu Jan 19 2023 Fedora Release Engineering <releng@fedoraproject.org> - 2.8.0-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_38_Mass_Rebuild

* Tue Nov 08 2022 Richard Shaw <hobbes1069@gmail.com> - 2.8.0-5
- Rebuild for yaml-cpp 0.7.0.

* Thu Jul 21 2022 Fedora Release Engineering <releng@fedoraproject.org> - 2.8.0-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_37_Mass_Rebuild

* Sun Jul 10 2022 Mamoru TASAKA <mtasaka@fedoraproject.org> - 2.8.0-3
- Rebuild for new gtest

* Tue Jun 28 2022 Benjamin A. Beasley <code@musicinmybrain.net> - 2.8.0-2
- Rebuilt for gtest 1.12.0 (close RHBZ#2101748)

* Tue May 24 2022 Neal Gompa <ngompa@fedoraproject.org> - 2.8.0-1
- Update to 2.8.0 (RH#2089779)

* Wed May 04 2022 Thomas Rodgers <trodgers@redhat.com> - 2.7.0-2
- Rebuilt for Boost 1.78

* Fri Feb 25 2022 Neal Gompa <ngompa@fedoraproject.org> - 2.7.0-1
- Update to 2.7.0 (RH#2058236)
- Backport fix for non-pch build
- Drop patches not needed for this release

* Thu Jan 20 2022 Fedora Release Engineering <releng@fedoraproject.org> - 2.6.0-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_36_Mass_Rebuild

* Mon Jan 03 2022 Neal Gompa <ngompa@fedoraproject.org> - 2.6.0-3
- Backport another fix for mir-devel FTI (RH#2036635)

* Tue Dec 21 2021 Neal Gompa <ngompa@fedoraproject.org> - 2.6.0-2
- Add patches to fix mir-devel FTI (RH#2034680)

* Tue Dec 21 2021 Neal Gompa <ngompa@fedoraproject.org> - 2.6.0-1
- Update to 2.6.0 (RH#1978675)

* Sun Nov 07 2021 Mamoru TASAKA <mtasaka@fedoraproject.org> - 2.4.0-6
- rebuild for new protobuf

* Tue Nov 02 2021 Mamoru TASAKA <mtasaka@fedoraproject.org> - 2.4.0-5
- rebuild against new liblttng-ust

* Tue Oct 26 2021 Adrian Reber <adrian@lisas.de> - 2.4.0-4
- Rebuilt for protobuf 3.18.1

* Fri Aug 06 2021 Jonathan Wakely <jwakely@redhat.com> - 2.4.0-3
- Rebuilt for Boost 1.76

* Thu Jul 22 2021 Fedora Release Engineering <releng@fedoraproject.org> - 2.4.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_35_Mass_Rebuild

* Thu Jun 24 2021 Neal Gompa <ngompa13@gmail.com> - 2.4.0-1
- Update to 2.4.0 (RH#1942812)

* Fri Jun 04 2021 Python Maint <python-maint@redhat.com> - 2.3.2-3
- Rebuilt for Python 3.10

* Tue Mar 30 2021 Jonathan Wakely <jwakely@redhat.com> - 2.3.2-2
- Rebuilt for removed libstdc++ symbol (#1937698)

* Thu Feb 11 2021 Neal Gompa <ngompa13@gmail.com> - 2.3.2-1
- Update to 2.3.2 (RH#1899510)
- Minor fixes to changelog entries

* Tue Jan 26 2021 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.0-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_34_Mass_Rebuild

* Fri Jan 22 2021 Jonathan Wakely <jwakely@redhat.com> - 2.1.0-6
- Rebuilt for Boost 1.75

* Wed Jan 13 2021 Adrian Reber <adrian@lisas.de> - 2.1.0-5
- Rebuilt for protobuf 3.14

* Fri Dec 04 2020 Jeff Law <law@redhat.com> - 2.1.0-4
- Fix more missing #includes for gcc-11

* Fri Oct 30 2020 Jeff Law <law@redhat.com> - 2.1.0-3
- Fix another missing #include for gcc-11

* Mon Oct 19 2020 Jeff Law <law@redhat.com> - 2.1.0-2
- Fix missing #includes for gcc-11

* Sat Oct 03 2020 Neal Gompa <ngompa13@gmail.com> - 2.1.0-1
- Update to 2.1.0 (RH#1883290)

* Thu Sep 24 2020 Adrian Reber <adrian@lisas.de> - 2.0.0.0-3
- Rebuilt for protobuf 3.13

* Tue Jul 28 2020 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.0.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Fri Jul 24 2020 Neal Gompa <ngompa13@gmail.com> - 2.0.0.0-1
- Rebase to 2.0.0.0 (RH#1860214)

* Sat Jul 18 2020 Neal Gompa <ngompa13@gmail.com> - 1.8.0-2
- Rebuilt for capnproto 0.8.0

* Sun Jul 12 2020 Neal Gompa <ngompa13@gmail.com> - 1.8.0-1
- Update to 1.8.0 (RH#1822993)

* Sun Jun 21 2020 Adrian Reber <adrian@lisas.de> - 1.7.1-3
- Rebuilt for protobuf 3.12

* Tue May 26 2020 Miro Hrončok <mhroncok@redhat.com> - 1.7.1-2
- Rebuilt for Python 3.9

* Wed Apr 01 2020 Neal Gompa <ngompa13@gmail.com> - 1.7.1-1
- Update to 1.7.1 (RH#1806678)
- Backport fixes from upstream to improve Mir functionality

* Mon Feb 17 2020 Neal Gompa <ngompa13@gmail.com> - 1.7.0-1
- Update to 1.7.0
- Backport fix from upstream to fix build with GCC 10 (RH#1799655)

* Wed Jan 29 2020 Fedora Release Engineering <releng@fedoraproject.org> - 1.6.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_32_Mass_Rebuild

* Thu Dec 05 2019 Neal Gompa <ngompa13@gmail.com> - 1.6.0-1
- Update to 1.6.0

* Fri Oct 18 2019 Richard Shaw <hobbes1069@gmail.com> - 1.5.0-2
- Rebuild for yaml-cpp 0.6.3.

* Fri Oct 11 2019 Neal Gompa <ngompa13@gmail.com> - 1.5.0-1
- Update to 1.5.0 (RH#1760820)

* Thu Oct 03 2019 Miro Hrončok <mhroncok@redhat.com> - 1.4.0-2
- Rebuilt for Python 3.8.0rc1 (#1748018)

* Tue Aug 27 2019 Neal Gompa <ngompa13@gmail.com> - 1.4.0-1
- Update to 1.4.0 (RH#1742690)

* Mon Aug 19 2019 Miro Hrončok <mhroncok@redhat.com> - 1.3.0-3
- Rebuilt for Python 3.8

* Thu Jul 25 2019 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Sun Jul 14 2019 Neal Gompa <ngompa13@gmail.com> - 1.3.0-1
- Update to 1.3.0 (RH#1678585)

* Fri Feb 01 2019 Fedora Release Engineering <releng@fedoraproject.org> - 1.1.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Wed Jan 30 2019 Jonathan Wakely <jwakely@redhat.com> - 1.1.0-2
- Rebuilt for Boost 1.69

* Sat Dec 22 2018 Neal Gompa <ngompa13@gmail.com> - 1.1.0-1
- Update to 1.1.0

* Tue Dec 04 2018 Igor Gnatenko <ignatenkobrain@fedoraproject.org> - 1.0.0-3
- Rebuild for new protobuf

* Fri Sep 28 2018 Neal Gompa <ngompa13@gmail.com> - 1.0.0-2
- Add patch to correctly detect gtest using pkg-config

* Sun Sep 23 2018 Neal Gompa <ngompa13@gmail.com> - 1.0.0-1
- Update to 1.0.0

* Tue Sep 11 2018 Neal Gompa <ngompa13@gmail.com> - 0.32.1-2
- Rebuild for gtest 1.8.1

* Sun Aug 26 2018 Neal Gompa <ngompa13@gmail.com> - 0.32.1-1
- Update to 0.32.1 (RH#1570223)

* Fri Jul 13 2018 Fedora Release Engineering <releng@fedoraproject.org> - 0.31.1-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Tue Jun 19 2018 Miro Hrončok <mhroncok@redhat.com> - 0.31.1-2
- Rebuilt for Python 3.7

* Sat Mar 31 2018 Neal Gompa <ngompa13@gmail.com> - 0.31.1-1
- Update to 0.31.1 (RH#1562271)
- Drop upstreamed patch to fix disabling LTO for ppc64 builds

* Wed Mar 21 2018 Neal Gompa <ngompa13@gmail.com> - 0.31.0.1-1
- Update to 0.31.0.1 (RH#1558534)
- Drop conditionals and scriptlets for Fedora < 28
- Add patch to fix disabling LTO for ppc64 builds
- Drop upstreamed patch for mir-perf-framework

* Mon Feb 19 2018 Neal Gompa <ngompa13@gmail.com> - 0.30.0.1-1
- Update to 0.30.0.1 (RH#1546741)

* Sat Feb 17 2018 Neal Gompa <ngompa13@gmail.com> - 0.30.0-1
- Update to 0.30.0 (RH#1545646)
- Switch to build with ninja
- Add patch to fix versioning of mir-perf-framework
- Switch to macroized ldconfig scriptlets

* Thu Feb 08 2018 Fedora Release Engineering <releng@fedoraproject.org> - 0.29.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Fri Jan 05 2018 Igor Gnatenko <ignatenkobrain@fedoraproject.org> - 0.29.0-2
- Remove obsolete scriptlets

* Sun Dec 17 2017 Neal Gompa <ngompa13@gmail.com> - 0.29.0-1
- Update to 0.29.0 (RH#1526660)
- Enable building and shipping tests for Fedora 27+

* Mon Nov 20 2017 Neal Gompa <ngompa13@gmail.com> - 0.28.1-1
- Initial import into Fedora (RH#1513512)

* Sat Nov 18 2017 Neal Gompa <ngompa13@gmail.com> - 0.28.1-0.3
- Add scriptlets for updating icon cache to mir-demos
- Add hicolor-icon-theme dependency to mir-demos
- Validate the desktop file shipped in mir-demos
- Declare which subpackages own Mir library subdirectories

* Fri Nov 17 2017 Neal Gompa <ngompa13@gmail.com> - 0.28.1-0.2
- Add patch to fix building with libprotobuf 3.4.1

* Wed Nov 15 2017 Neal Gompa <ngompa13@gmail.com> - 0.28.1-0.1
- Rebase to 0.28.1
- Add patch to fix installing the perf framework
- Add patch to fix locating Google Mock for building Mir

* Sat Apr 15 2017 Neal Gompa <ngompa13@gmail.com> - 0.26.2-0.1
- Rebase to 0.26.2

* Wed Nov  9 2016 Neal Gompa <ngompa13@gmail.com> - 0.24.1-0.2
- Add patch to add missing xkbcommon Requires to mirclient.pc

* Mon Oct 31 2016 Neal Gompa <ngompa13@gmail.com> - 0.24.1-0.1
- Initial packaging
