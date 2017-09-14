#include <boost/log/core.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/keywords/facility.hpp>
#include <boost/log/keywords/use_impl.hpp>
#include "logging.hpp"

namespace logging = boost::log;
namespace sinks = logging::sinks;
namespace keywords = logging::keywords;

namespace sp
{

namespace str {
  static const char log_format[] = "%TimeStamp% [%Severity%][%ThreadID%]: %Message%";
}

typedef sinks::synchronous_sink< sinks::syslog_backend > sink_t;

void init_native_syslog()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Create a backend
    boost::shared_ptr< sinks::syslog_backend > backend(new sinks::syslog_backend(
      keywords::facility = sinks::syslog::local0,
      keywords::use_impl = sinks::syslog::native
    ));

    // Set the straightforward level translator for the "Severity" attribute of type int
    backend->set_severity_mapper(sinks::syslog::direct_severity_mapping< int >("Severity"));

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    core->add_sink(boost::make_shared< sink_t >(backend));
}

void setup_logging(const std::string &log_path, logging::trivial::severity_level log_level) {
  logging::register_simple_formatter_factory<logging::trivial::severity_level, char>("Severity");
   if(log_path.empty()) {
     init_native_syslog();
   }
   else {
     logging::add_file_log(
       keywords::file_name = log_path,
       keywords::open_mode = std::ios_base::app,
       keywords::format = str::log_format
     );
   }
   logging::add_common_attributes();

   logging::core::get()->set_filter(
     logging::trivial::severity >= log_level
   );
}

}
