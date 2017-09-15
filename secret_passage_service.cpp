#include <iostream>
#include <stdexcept>
#include "secret_passage_service.hpp"
#include "logging.hpp"

namespace sp
{

secret_passage_service::secret_passage_service(settings st)
  : st_(st)
{}

int secret_passage_service::run() {
  try {
    setup_logging(st_.log_path, st_.log_level);
    LOG_INFO << "##### secret passage new log entry #####";
  }
  catch(const std::exception& ex) {
    std::cout << "Exception while setting up logging: " << ex.what() << std::endl;
    std::cout.flush();
    return -1;
  }

  LOG_INFO << "starting with options:\n" << st_.to_string();

  if(-1 == daemon(0, 0)) {
    LOG_FATAL << "service failed to daemonize";
    return -2;
  }

  LOG_DEBUG << "successfully daemonized, pid is " << getpid();
  if(!pid_.open(st_.pid_path.c_str())) {
    return -3;
  }

  LOG_INFO << "serice stopped";
  return 0;
}



}
