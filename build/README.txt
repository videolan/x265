To compile x265 you must first install cmake and then invoke
cmake from here with the location of the root CMakeLists.txt
file in the source folder.  In other words run:

cmake ..\source

cmake will generate Makefiles or Solution files as required
by your default compiler.

See the CMAKE documentation on how to override the default
compiler selection and other configurables.


MSYS example

To build x265 with GCC on Windows, you must install MinGW and
select the MSYS developer environment install option.  Open
an msys shell (MinGW/msys/1.0/msys.bat) and navigate to this
build folder:

$ cd /c/repos/x265/build
$ cmake -G "MSYS Makefiles" ../source
$ make

You should now have an encoder.exe in build/App/TAppEncoder/
