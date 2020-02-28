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
