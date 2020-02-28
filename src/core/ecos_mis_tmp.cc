#include <iostream>

#include <logger/Logger.hpp>
#include <logger/CoutSink.hpp>

#include "config_opts.h"
#include "global.h"

using namespace std;
using namespace ecos_mis;

int main()
{
    Logger::addSink(new Logger::detail::CoutSink());
    Logger::setLogLevel(FLAGS_log_level);

    LOG(INFO) << "hello world";
    return 0;
}