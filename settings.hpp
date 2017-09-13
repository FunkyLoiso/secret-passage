#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "exception.hpp"

namespace sp {

class settings {
public:
  typedef sp::exception<settings> exception;
  typedef sp::exception<struct help_requested_tag> help_requested;

  static void show_help();

  settings();
  void read(int argc, char* argv[]); // throws exception, help_requested
  void clear();

  std::string pid_path;
  // ...
};

}

#endif // SETTINGS_HPP
