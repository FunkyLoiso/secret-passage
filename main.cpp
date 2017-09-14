#include <iostream>
#include "settings.hpp"
#include "secret_passage_service.hpp"

int main(int argc, char *argv[]) {
  sp::settings st;
  try {
    st.read(argc, argv);
  }
  catch(const sp::settings::help_requested&) {
    sp::settings::show_help();
    return 0;
  }
  catch(const std::exception& ex) {
    std::cout << "Exception: " << ex.what() << std::endl;
    sp::settings::show_help();
    return -1;
  }

  sp::secret_passage_service svc(st);
  return svc.run();
}
