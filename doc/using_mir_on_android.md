Using Mir on an Android device {#using_mir_on_android}
==============================

After installing mir on your device (see \ref installing_prebuilt_on_android),
open the Android shell:

    $ adb root
    $ adb shell

In the Android shell, stop SurfaceFlinger and open the Ubuntu touch chroot shell:

    # stop
    # ubuntu_chroot shell

Now, start Mir and a client application:

    # mir &
    # some-mir-client (e.g. mir_demo_client_accelerated)

Getting some example client applications
----------------------------------------

You can get some example programs by installing the `libmirclient-demos` package
inside the Ubuntu touch chroot shell (see above):

    # apt-get install libmirclient-demos
