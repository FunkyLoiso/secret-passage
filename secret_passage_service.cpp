#include <iostream>
#include "secret_passage_service.hpp"
#include "logging.hpp"

namespace sp
{

secret_passage_service::secret_passage_service(settings st)
  : st_(st)
{

}

void secret_passage_service::run() {
  try {
    setup_logging(st_.log_path, st_.log_level);
  }
  catch(const std::exception& ex) {
    std::cout << "Exception while setting up logging: " << ex.what() << std::endl;
    std::cout.flush();
    exit(-1);
  }

  LOG_INFO << "starting with options:\n" << st_.to_string();
}



}
