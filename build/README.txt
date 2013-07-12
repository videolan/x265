= Mandatory Prerequisites =

* GCC, MSVC (9, 10, or 11), or Intel C/C++
* CMake 2.6 or later http://www.cmake.org
* On linux, ccmake is helpful, usually a package named cmake-curses-gui 


= Optional Prerequisites =

1. Yasm 1.2.0 or later, to compile assembly primitives (performance)

   For Windows, download
   http://www.tortall.net/projects/yasm/releases/yasm-1.2.0-win32.exe or
   http://www.tortall.net/projects/yasm/releases/yasm-1.2.0-win64.exe
   depending on your O/S and copy the EXE into C:\Windows or somewhere else
   in your %PATH% that a 32-bit app (cmake) can find it. If it is not in the
   path, you must manually tell cmake where to find it.

   For Linux, yasm-1.2.0 is likely too new to be packaged for your system so you
   will need get http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz
   compile, and install it.

   Once YASM is properly installed, run cmake to regenerate projects. If you
   do not see the below line in the cmake output, YASM is not in the PATH.

   -- Found Yasm 1.2.0 to build assembly primitives

   Now build the encoder and run x265 -V. If you see "assembly" on this
   line, you have YASM properly installed:

   x265 [info]: performance primitives: intrinsic assembly

2. VisualLeakDetector (Windows Only)

   Download from https://vld.codeplex.com/releases and install. May need
   to re-login in order for it to be in your %PATH%.  Cmake will find it
   and enable leak detection in debug builds without any additional work.

   If VisualLeakDetector is not installed, cmake will complain a bit, but
   it is completely harmless.


= Build Instructions Linux =

1. Use cmake to generate Makefiles: cmake ../source
2. Build x265:                      make

  Or use our shell script which runs cmake then opens the curses GUI to
  configure build options

1. cd build/linux ; ./make-Makefiles.bash
2. make


= Build Instructions Windows =

We recommend you use one of the make-solutions.bat files in the appropriate
build/ sub-folder for your preferred compiler.  They will open the cmake-gui
to configure build options, click configure until no more red options remain,
then click generate and exit.  There should now be an x265.sln file in the
same folder, open this in Visual Studio and build it.

= Version number considerations =

Note that cmake will update X265_VERSION each time cmake runs, if you are
building out of a Mercurial source repository.  If you are building out of
a release source package, the version will not change.  If Mercurial is not
found, the version will be "unknown".
