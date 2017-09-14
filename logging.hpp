#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <string>
#include "settings.hpp"
#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

namespace sp
{

void setup_logging(const std::string& log_path, boost::log::trivial::severity_level log_level);

}

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(__logger, boost::log::sources::severity_logger<boost::log::trivial::severity_level>)
#define LOG_TRACE BOOST_LOG_SEV(__logger::get(), boost::log::trivial::trace)
#define LOG_DEBUG BOOST_LOG_SEV(__logger::get(), boost::log::trivial::debug)
#define LOG_INFO BOOST_LOG_SEV(__logger::get(), boost::log::trivial::info)
#define LOG_WARNING BOOST_LOG_SEV(__logger::get(), boost::log::trivial::warning)
#define LOG_ERROR BOOST_LOG_SEV(__logger::get(), boost::log::trivial::error)
#define LOG_FATAL BOOST_LOG_SEV(__logger::get(), boost::log::trivial::fatal)


#endif // LOGGING_HPP
