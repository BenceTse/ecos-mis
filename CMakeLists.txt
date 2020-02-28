
#cmake所需最小版本
cmake_minimum_required(VERSION 3.0)

SET(CMAKE_INSTALL_PREFIX /usr)

# PROJECT(projectname [CXX] [C] [Java])指定项目名称和工程支持语言
project(ecos-mis CXX C ASM)

# 看看用什么编译器
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
  message("clang++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  message("g++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
endif()

# 看看子模块src/cpp-httplib里的文件是否存在，如果不存在则报错
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/httplib.h")
#     message("${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/httplib.h EXISTS")
# else()
#     message("${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/httplib.h Not EXISTS, git submodule update?")
# endif()

option(WITH_STATIC_LIBSTDCXX "Link against libstdc++ statically" OFF)
if(WITH_STATIC_LIBSTDCXX)
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(static_linker_flags "-static-libstdc++ -static-libgcc")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${static_linker_flags}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${static_linker_flags}")
        unset(static_linker_flags)
        set(GPERFTOOLS_USE_STATIC_LIBS TRUE)
    else()
        message(FATAL_ERROR "Please use GCC to enable WITH_STATIC_LIBSTDCXX")
    endif()
endif()

# CMAKE_CURRENT_SOURCE_DIR 指的是当前处理的 CMakeLists.txt 所在的路径
set(PROJ_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)


# 将所有的依赖库以及二进制程序放到同一个地方
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")


find_package(PkgConfig)

option(WITH_GFLAGS "Gflags is here" ON)
if(WITH_GFLAGS)
  find_package(gflags REQUIRED)
  set(HAVE_LIBGFLAGS ${GFLAGS_FOUND})
endif()


option(WITH_MYSQLPP "Mysqlpp is here" ON)
if(WITH_MYSQLPP)
  find_package(mysqlpp REQUIRED)
  set(HAVE_LIBMYSQLPP ${MYSQLPP_FOUND})
  include_directories(${MYSQLPP_INCLUDE_DIR})
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -fPIC")

#-Wall显示所有警告 -g添加gdb调试支持
set(CMAKE_CXX_FLAGS
    " -std=c++11 -g  -Wall -D_FILE_OFFSET_BITS=64 -Wno-sign-compare -rdynamic -gdwarf-2")
# CXX的flags也要包含C的flags（主要是fPIC）
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")

#string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#不做优化
# set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

#INCLUDE_DIRECTORIES([AFTER|BEFORE] [SYSTEM] dir1 dir2 ...)
#这条指令可以用来向工程添加多个特定的头文件搜索路径，路径之间用空格分割
include_directories(${PROJ_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#添加子文件夹
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(googletest)
exec_program("cp compile_commands.json ../")

