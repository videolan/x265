To compile xhevc you must first install cmake and then invoke
cmake from here with the location of the root CMakeLists.txt
file in the source folder.  In other words run:

cmake ..\source

cmake will generate Makefiles or Solution files as required
by your default compiler.

See the CMAKE documentation on how to override the default
compiler selection and other configurables.
