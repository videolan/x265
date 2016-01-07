# CMake toolchain file for cross compiling x265 for ARM arch

set(CROSS_COMPILE_ARM 1)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv6l)

# specify the cross compiler
set(CMAKE_C_COMPILER arm-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabi-g++)

# specify the target environment
SET(CMAKE_FIND_ROOT_PATH  /usr/arm-linux-gnueabi)
