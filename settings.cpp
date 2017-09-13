#include <iostream>
#include <boost/program_options.hpp>
#include "settings.hpp"

namespace po = boost::program_options;

namespace sp
{

namespace def {
}

void describe(po::options_description& desc, settings* st = nullptr) {
  desc.add_options()
    ("help,h", "print this message and exit")
    ("pid-path,p", po::value<std::string>(st ? &st->pid_path : nullptr), "full path to pid file");
}

void settings::show_help() {
  po::options_description dummy;
  describe(dummy);
  std::cout << dummy << std::endl;
}

settings::settings() {
  clear();
}

void settings::read(int argc, char *argv[]) {
  po::options_description opts;
  describe(opts, this);

  po::variables_map map;
  try {
    auto parsed = po::parse_command_line(argc, argv, opts);
    po::store(parsed, map);
    po::notify(map);
    if(map.count("help")) {
      throw help_requested();
    }
  }
  catch(const po::error& er) {
    throw exception("Failed to parse command line", er.what());
  }

  if(!map.count("pid-path")) {
    throw exception("Option 'pid-path' is mandatory");
  }
}

void settings::clear() {
  pid_path.clear();
}

}

