# 当前目录作为头文件的（根）目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(core)
add_subdirectory(logger)
add_subdirectory(base)
add_subdirectory(netbase)
