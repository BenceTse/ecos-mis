
# set(NETBASE_SRCS
#     EventLoop.cc
#     Timestamp.cc
#     Channel.cc
#     )
aux_source_directory(. NETBASE_SRCS)

add_library(networking
    ${NETBASE_SRCS}
    )
# target_link_libraries(networking arbibase)
