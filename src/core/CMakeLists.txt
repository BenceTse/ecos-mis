

add_executable(ecos_mis_tmp
    ecos_mis_tmp.cc
    config_opts.cc
    )

target_link_libraries(ecos_mis_tmp
    logger
    base
    networking
    gflags
    )

# install(TARGETS interface_server DESTINATION bin)
