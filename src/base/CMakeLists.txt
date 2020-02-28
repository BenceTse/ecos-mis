
set(ARBIBASE_SRCS
    # ICache.cc
    utils.cc
    # ProtobufCodec.cc
    Thread.cc
    # ThreadPool.cc
    CountDownLatch.cc
    Timestamp.cc
    )

add_library(base
    ${ARBIBASE_SRCS}
    )
target_link_libraries(base pthread)
# target_link_libraries(arbibase pthread messages arbilogger)
#set_target_properties(arbibase PROPERTIES COMPILE_FLAGS "-I${FUSE_INCLUDE_DIRS}")

