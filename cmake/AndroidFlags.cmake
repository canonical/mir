function(get_android_flags)
  
  set(MIR_NDK_PATH $ENV{MIR_NDK_PATH})
  
  #build flags for finding things in the sysroot
  set( ANDROID_CXX_FLAGS "${ANDROID_CXX_FLAGS} --sysroot=${MIR_NDK_PATH}/sysroot")
  set( ANDROID_CXX_FLAGS "${ANDROID_CXX_FLAGS} -isystem ${MIR_NDK_PATH}/sysroot/usr/include")
  set( ANDROID_CXX_FLAGS "${ANDROID_CXX_FLAGS} -isystem ${MIR_NDK_PATH}/include/c++")
  set( ANDROID_CXX_FLAGS "${ANDROID_CXX_FLAGS} -isystem ${MIR_NDK_PATH}/include/c++/arm-linux-androideabi/armv7-a")

  #note: this has to go away before landing. works around a STL header problem
  set( ANDROID_CXX_FLAGS "${ANDROID_CXX_FLAGS} -fpermissive")

  set( ANDROID_CXX_FLAGS "${ANDROID_CXX_FLAGS}" PARENT_SCOPE)

  #compile definitions
  set( ANDROID_DEFINITIONS 
    -D_GLIBCXX_HAS_GTHREADS;
    -D_GLIBCXX_USE_C99_STDINT_TR1;
    -DHAVE_ANDROID_OS=1;
    -D_GLIBCXX_USE_SCHED_YIELD;
    -DHAVE_FUTEX; 
    -DHAVE_FUTEX_WRAPPERS=1; 
    -DHAVE_FORKEXEC; 
    -DHAVE_OOM_ADJ; 
    -DHAVE_ANDROID_IPC; 
    -DHAVE_POSIX_FILEMAP; 
    -DHAVE_TERMIO_H; 
    -DHAVE_SYS_UIO_H; 
    -DHAVE_SYMLINKS; 
    -DHAVE_IOCTL; 
    -DHAVE_POSIX_CLOCKS; 
    -DHAVE_TIMEDWAIT_MONOTONIC; 
    -DHAVE_EPOLL; 
    -DHAVE_ENDIAN_H; 
    -DHAVE_LITTLE_ENDIAN; 
    -DHAVE_BACKTRACE=0; 
    -DHAVE_DLADDR=0; 
    -DHAVE_CXXABI=0; 
    -DHAVE_SCHED_SETSCHEDULER; 
    -D__linux__; 
    -DHAVE_MALLOC_H; 
    -DHAVE_LINUX_LOCAL_SOCKET_NAMESPACE=1; 
    -DHAVE_INOTIFY=1; 
    -DHAVE_MADVISE=1; 
    -DHAVE_TM_GMTOFF=1; 
    -DHAVE_DIRENT_D_TYPE=1; 
    -DARCH_ARM; 
    -DOS_SHARED_LIB_FORMAT_STR="lib%s.so"; 
    -DHAVE__MEMCMP16=1; 
    -DMINCORE_POINTER_TYPE="unsigned char *"; 
    -DHAVE_SA_NOCLDWAIT; 
    -DOS_PATH_SEPARATOR='/';
    -DOS_CASE_SENSITIVE; 
    -DHAVE_SYS_SOCKET_H=1; 
    -DHAVE_PRCTL=1; 
    -DHAVE_WRITEV=1 ;
    -DHAVE_STDINT_H=1 ;
    -DHAVE_STDBOOL_H=1 ;
    -DHAVE_SCHED_H=1 ;
    -D__STDC_INT64__;
    -DANDROID_SMP;
  
    -DANDROID;
    -DGTEST_OS_LINUX_ANDROID;
    -DGTEST_HAS_CLONE=0;
    -DGTEST_HAS_POSIX_RE=0;
    -DGTEST_HAS_PTHREAD=0
  
    PARENT_SCOPE
    )

  #linker flags
  set(ANDROID_LINKER_FLAGS "-Wl,-rpath,${MIR_NDK_PATH}/sysroot/usr/lib:${MIR_NDK_PATH}/arm-linux-androideabi/lib" PARENT_SCOPE)

  set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -Wl,-T,/opt/stand-orig/arm-linux-androideabi/lib/ldscripts/armelf_linux_eabi.xsc")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -Wl,--gc-sections")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -Wl,-shared,-Bsymbolic")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -Wl,-z,noexecstack")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -Wl,--icf=safe")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -Wl,--fix-cortex-a8")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -Wl,--no-undefined")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -nostdlib")

   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -lc")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -lstdc++")
   set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} -lsupc++")

    set(ANDROID_SO_CRTBEGIN "/opt/stand-orig/sysroot/usr/lib/crtbegin_so.o" PARENT_SCOPE)
  
    set(ANDROID_SO_LINKER_FLAGS "${ANDROID_SO_LINKER_FLAGS} /opt/stand-orig/sysroot/usr/lib/crtbegin_so.o" PARENT_SCOPE)
    


    set(ANDROID_SO_CRTEND "/opt/stand-orig/sysroot/usr/lib/crtend_so.o" PARENT_SCOPE)
    set(ANDROID_SO_GCC "/opt/stand-orig/./lib/gcc/arm-linux-androideabi/4.6.x-google/libgcc.a" PARENT_SCOPE)
    set(ANDROID_SO_LIBRARIES protobuf;c;m;stdc++;supc++ PARENT_SCOPE)
endfunction(get_android_flags)
