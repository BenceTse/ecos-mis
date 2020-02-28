
set(ARBILOGGER_SRCS
    Buffer.cc
    AsynLogging.cc
    LogStream.cc
    Logger.cc
    )

add_library(logger
    ${ARBILOGGER_SRCS}
    )
# target_link_libraries(logger)
# set_target_properties(logger PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../")
