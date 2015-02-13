Setup VMware for Mir {#setup_vmware_for_mir}
============================================

Here is a quick-start guide for running Mir in VMware:

1. Go to https://my.vmware.com/web/vmware/downloads and download
   VMware Player (last in Desktop & End-User Computing section).

2. Install VMware player with: sudo bash VMware-Player-<VERSION>.x86_64.bundle

3. Get the latest vivid daily iso (*not* the unity-next iso!).

4. Install vivid into VM, restart and log in.

5. Install VMware tools from the menu "Virtual Machine->Install VMWare tools"
   and follow the instructions. If you get an error "VMware Tools installation
   cannot be started manually while the easy install is in progress
   see [1].

6. If you are using Mesa drivers, stop the virtual machine, go to the folder
   where the VM is saved and add the following line to the <vm-name>.vmx file
   there:

       mks.gl.allowBlacklistedDrivers = "TRUE"

7. Restart the VM, log in and try glmark2 to ensure it shows 'SVGA' as the
   GL renderer (*not* 'llvmpipe'!):

       $ sudo apt-get install glmark2
       $ glmark2

8. sudo apt-get unity8-desktop-session-mir mir-demos

9. Try out a demo mir server:
   1. Change to VT2 and run "sudo mir_proving_server"
   2. Change to VT3 and run "sudo mir_demo_client_egltriangle"
   3. Change back to VT2, you should see a window with a triangle spinning fast
   4. Use Alt + click + drag to move the window around
   5. Use Ctrl-Alt-Backspace to stop the server

10. Try out the unity8 desktop session by selecting it in lightdm

Unfortunately the VMware KMS support is still somewhat unstable. The KMS/DRM
driver doesn't seem to respect vsync (hence the fast spinning egl triangle in
step 9c). Also switching between VTs may occasionally make the VM unresponsive,
and sometimes screen resizing causes problems.  I strongly suggest you set up
an ssh server in the VM so you can inspect the system over ssh from the host.

[1] http://kb.vmware.com/selfservice/microsites/search.do?language=en_US&cmd=displayKC&externalId=1017687
