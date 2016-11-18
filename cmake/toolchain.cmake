# this one is important
set(CMAKE_SYSTEM_NAME Linux)
#this one not so much
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)

# specify the cross compiler
set(CMAKE_C_COMPILER ${CMAKE_CURRENT_LIST_DIR}/bb-kernel/dl/gcc-linaro-4.9-2014.11-x86_64_arm-linux-gnueabihf/bin/${CMAKE_LIBRARY_ARCHITECTURE}-gcc)
set(CMAKE_CXX_COMPILER ${CMAKE_CURRENT_LIST_DIR}/bb-kernel/dl/gcc-linaro-4.9-2014.11-x86_64_arm-linux-gnueabihf/bin/${CMAKE_LIBRARY_ARCHITECTURE}-g++)
set(CMAKE_SYSROOT ${CMAKE_CURRENT_LIST_DIR}/rootfs/)
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath-link,${CMAKE_SYSROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE}:${CMAKE_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-rpath-link,${CMAKE_SYSROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE}:${CMAKE_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}")

# where is the target environment
set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH}
    ${CMAKE_CURRENT_LIST_DIR}/rootfs/ )
#    ${CMAKE_CURRENT_LIST_DIR}/ti/linux-devkit/sysroots/cortexa8hf-vfp-neon-linux-gnueabi/)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}
    ${CMAKE_CURRENT_LIST_DIR}/../dk_utils/)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
