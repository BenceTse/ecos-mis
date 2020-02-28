add_executable(test_mysql test_mysql.cc ../src/core/config_opts.cc)
target_link_libraries(test_mysql ${MYSQLPP_LIBRARIES} gflags gtest)

