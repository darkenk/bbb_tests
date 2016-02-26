# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER $ENV{HOME}/workspace/projects/bbb/bb-kernel/dl/gcc-linaro-4.9-2014.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER $ENV{HOME}/workspace/projects/bbb/bb-kernel/dl/gcc-linaro-4.9-2014.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  $ENV{HOME}/workspace/projects/bbb/rootfs/ $ENV{HOME}/workspace/projects/bbb/ti/linux-devkit/sysroots/cortexa8hf-vfp-neon-linux-gnueabi/)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
