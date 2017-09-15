#include <iostream>
#include <stdexcept>
#include "secret_passage_service.hpp"
#include "logging.hpp"
#include "create_tap.hpp"

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
  std::cout << "starting Secret Passage daemon in '" << settings::mode::name(st_.mode_) << "' mode" << std::endl;
  LOG_INFO << bf("starting in '%s' mode with options:\n%s")
    % settings::mode::name(st_.mode_) % st_.to_string();

  if(-1 == daemon(0, 0)) {
    LOG_FATAL << "service failed to daemonize";
    return -2;
  }
  LOG_DEBUG << "successfully daemonized, pid is " << getpid();

  if(!st_.pid_path.empty() && !pid_.open(st_.pid_path.c_str())) {
    return -3;
  }

  std::string tap_name;
  tap_ = create_tap(&tap_name);
  if(!tap_->valid()) {
    return -4;
  }
  LOG_INFO << bf("tap interface created: '%s'") % tap_name;

  LOG_INFO << "Secret Passage serice stopped";
  return 0;
}



}
