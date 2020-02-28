#pragma once
#include <gflags/gflags.h>

DECLARE_int32(log_level);
DECLARE_string(log_file_name);
DECLARE_int32(http_time_out);

DECLARE_string(amqp_server_host);
DECLARE_int32(amqp_server_port);
DECLARE_string(amqp_server_username);
DECLARE_string(amqp_server_passwd);
DECLARE_string(amqp_server_vhost);

//数据库相关参数
DECLARE_string(db_name);
DECLARE_string(db_ip);
DECLARE_int32(db_port);
DECLARE_string(db_user);
DECLARE_string(db_password);
