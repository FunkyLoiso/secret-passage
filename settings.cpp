#include <iostream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "settings.hpp"

namespace po = boost::program_options;

namespace sp
{

namespace str {
namespace severity {
  using namespace boost::log::trivial;
  static const std::string trace(to_string(severity_level::trace));
  static const std::string debug(to_string(severity_level::debug));
  static const std::string info(to_string(severity_level::info));
  static const std::string warning(to_string(severity_level::warning));
  static const std::string error(to_string(severity_level::error));
  static const std::string fatal(to_string(severity_level::fatal));
}
}

namespace def {
  static const auto log_level = str::severity::warning;
}

void describe(po::options_description& desc, settings* st = nullptr) {
  auto severity_handler = [st](const std::string sv) {
    if(!st) return;
    if(boost::iequals(sv, str::severity::trace)) st->log_level = boost::log::trivial::trace;
    else if(boost::iequals(sv, str::severity::debug)) st->log_level = boost::log::trivial::debug;
    else if(boost::iequals(sv, str::severity::info)) st->log_level = boost::log::trivial::info;
    else if(boost::iequals(sv, str::severity::warning)) st->log_level = boost::log::trivial::warning;
    else if(boost::iequals(sv, str::severity::error)) st->log_level = boost::log::trivial::error;
    else if(boost::iequals(sv, str::severity::fatal)) st->log_level = boost::log::trivial::fatal;
  };

  desc.add_options()
    ("help,h", "print this message and exit")
    ("pid-path,P", po::value<std::string>(st ? &st->pid_path : nullptr), "full path to pid file")
    ("config-path,C", po::value<std::string>(), "full path to config file")
    ("log-path,L", po::value<std::string>(st ? &st->log_path : nullptr), "full path to log file. syslog if absent")
    ("log-level,l", po::value<std::string>()->notifier(severity_handler)->default_value(def::log_level), "log level: trace, debug, info, warning, error or fatal")
    ;
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
  auto parsed = po::parse_command_line(argc, argv, opts);
  po::store(parsed, map);

  auto cfg_i = map.find("config-path");
  if(cfg_i != map.end()) {
      auto cfg_parsed = po::parse_config_file<char>(cfg_i->second.as<std::string>().c_str(), opts);
      po::store(cfg_parsed, map);
  }

  try {
    po::notify(map);
  }
  catch(const po::error& er) {
    throw exception("Failed to parse command line", er.what());
  }

  if(map.count("help")) {
    throw help_requested();
  }

  if(!map.count("pid-path")) {
    throw exception("Option 'pid-path' is mandatory");
  }
}

void settings::clear() {
  pid_path.clear();
}

std::string settings::to_string() const {
  std::stringstream sstr;
#define __W(arg) sstr << '\t' << #arg << ": " << arg << '\n'
  __W(pid_path);
  __W(log_path);
  sstr << "\tlog_level: " << boost::log::trivial::to_string(log_level) << '\n';
#undef __W
  return sstr.str();
}


}

