Setup KVM for Mir {#setup_kvm_for_mir}
=================

At the time of writing not all necessary patches to run Mir inside KVM can be
found in a vanilla kernel release. The steps below describe setting up KVM and
installing the right version of Mesa and the Linux kernel.

Install KVM and utilities
-------------------------

The only way to run Mir inside KVM requires a setup with
[SPICE](http://spice-space.org). So next to KVM a SPICE client is needed on
the host to provide a monitor to the guest. The following description will use
virt-manager for this task:

    $ apt-get install qemu-KVM virt-manager python-spice-client-gtk

More details on setting up KVM can be found in the
[wiki](https://help.ubuntu.com/community/KVM/Installation).

Now create a new virtual machine with virt-manager and a current Linux boot iso 
image or reconfigure an existing installation.

Configure the virtual machine
-----------------------------

Launch virt-manager and open your virtual machine. Go to the configuration
options through the info icon in the toolbar. There are now two relevant
configuration entries: Video and Display.

* Open the Display settings and select SPICE instead of KVM
* Open the Video settings and select QXL as model. 

Now boot the machine and build a new kernel and verify that you have the right
Mesa package.

Build the right kernel
----------------------

Until Linux 3.18 is released either build 
[drm-next](http://cgit.freedesktop.org/~airlied/linux/log/?h=drm-next) for your
KVM architecture or pull the sources of the current Ubuntu kernel image and
apply the following two patches before you build:

* [drm/qxl: simple crtc page flipping emulated using buffer copy]
(http://cgit.freedesktop.org/~airlied/linux/patch/?id=058e9f5c8236ad740ab984588b507758e5feee6d)
* [drm/qxl: enables gem prime helpers for qxl using dummy driver callbacks]
(http://cgit.freedesktop.org/~airlied/linux/patch/?id=47c1296829505d119d7d58dd23d39cc5db344f12)

More detais on building a Kernel within and for Ubuntu is described [here]
(https://help.ubuntu.com/community/Kernel/Compile) in full detail.


Verify Mesa installation
------------------------

Since the DRM QXL driver only provides KMS, GEM and dma-buf support, and no 3D
GPU emulation or forwarding, Mesa will load the kms-swrast driver. This driver
is available since Mesa 10.3.0
With that we have enough support for EGL and GLESv2 to run Mir.

Additional steps
----------------

This is not necessary but helpful for day to day use:

* Set up sharing on file system level using 9p as shown in
[Fileystem Passthrough](http://www.linux-kvm.org/page/9p_virtio)

Now you can finally install & run unity8-desktop-session-mir in KVM.
