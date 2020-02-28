
#include "config_opts.h"

DEFINE_int32(log_level, 1, "level of logger");
DEFINE_string(log_file_name, "./ecos2.log", "the name of log file");
DEFINE_int32(http_time_out, 4, "the timeout of http connection");

DEFINE_string(amqp_server_host, "127.0.0.1", "the host of amqp server");
DEFINE_int32(amqp_server_port, 5672, "the port of amqp server"); // 5672
// DEFINE_string(amqp_server_host,"219.223.192.62","the host of amqp server");
// DEFINE_int32(amqp_server_port, 5672,"the port of amqp server");//5672
// DEFINE_string(amqp_server_host,"219.223.197.61","the host of amqp server");
// DEFINE_int32(amqp_server_port, 1104,"the port of amqp server");//5672
DEFINE_string(amqp_server_username, "guest", "the username of amqp server");
DEFINE_string(amqp_server_passwd, "guest", "the passwd of amqp server");
DEFINE_string(amqp_server_vhost, "/", "the vhost of amqp server");

// 数据库相关
DEFINE_string(db_name, "ecos", "db name");
DEFINE_string(db_ip, "127.0.0.1", "db ip");
DEFINE_int32(db_port, 3306, "db port");
DEFINE_string(db_user, "ecos-v2", "db user");
DEFINE_string(db_password, "ecos-v2", "db password");
