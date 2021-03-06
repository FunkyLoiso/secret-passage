#include <iostream>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/placeholders.hpp>
#include "secret_passage_service.hpp"
#include "logging.hpp"
#include "create_tap.hpp"

namespace asio = boost::asio;

namespace sp
{

secret_passage_service::secret_passage_service(settings st)
  : st_(st)
{}

int secret_passage_service::run() {
  try {
    setup_logging(st_);
    LOG_INFO << "##### secret passage new log entry #####";
  }
  catch(const std::exception& ex) {
    std::cout << "Exception while setting up logging: " << ex.what() << std::endl;
    std::cout.flush();
    return -10;
  }
  std::cout << "starting Secret Passage daemon in '" << settings::mode::name(st_.mode) << "' mode" << std::endl;
  LOG_INFO << bf("starting in '%s' mode with options:\n%s")
    % settings::mode::name(st_.mode) % st_.to_string();

  if(st_.daemonize && -1 == daemon(0, 0)) {
    LOG_FATAL << "service failed to daemonize";
    return -20;
  }

  if(!st_.pid_path.empty() && !pid_.open(st_.pid_path.c_str())) {
    return -30;
  }

  std::string tap_name;
  tap_ = create_tap(&tap_name);
  if(!tap_->valid()) {
    return -40;
  }
  LOG_INFO << bf("tap interface created: '%s'") % tap_name;

  if(settings::mode::listen == st_.mode) {
    listen_mode_ = boost::make_shared<listen_mode>(ios_, st_, tap_);
    listen_mode_->setup();
  }
  else {
    connect_mode_ = boost::make_shared<connect_mode>(ios_, st_, tap_);
    connect_mode_->setup();
  }
  try {
    asio::signal_set stop_signals(ios_, SIGTERM, SIGINT);
    stop_signals.async_wait(boost::bind(
      &secret_passage_service::handle_stop,
        this,
        asio::placeholders::error
    ));
    ios_.run();
  }
  catch(const std::exception& ex) {
    LOG_FATAL << bf("Exception: %s") % ex.what();
    return -70;
  }

  LOG_INFO << "Secret Passage serice stopped";
  return 0;
}

void secret_passage_service::handle_stop(const boost::system::error_code& ec) {
  if(ec) {
    LOG_ERROR << "secret_passage_service::handle_stop: " << ec.message();
  }
  ios_.stop();
}

}
