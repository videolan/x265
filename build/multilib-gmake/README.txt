These two subfolders can be used to build a single x265 console binary
on linux or any other system supporting 'make' builds, with both the
8bit and 10bit libraries statically linked together.

At the end of the process, the 8bit/libx265.a and 10bit/libx265.a could
also be linked into other applications, so long as they use
x265_api_get() or x265_api_query() to acquire the x265 API and then only
use the function pointers and data elementes within (no other C APIs or
symbols are exported).

The folders must be built in a specific order. 10bit first, then 8bit
last. ie:

cd 10bit
./build.sh
cd ../8bit
./build.sh
