#ifndef SECRET_PASSAGE_SERVICE_HPP
#define SECRET_PASSAGE_SERVICE_HPP

#include <boost/asio/io_service.hpp>
#include <boost/shared_ptr.hpp>
#include "settings.hpp"
#include "pid_file.hpp"
#include "scoped_descriptor.hpp"
#include "listen_mode.hpp"

namespace sp
{

class secret_passage_service
{
public:
  secret_passage_service(settings st);
  int run();

private:
  void handle_stop(const boost::system::error_code& ec);

  pid_file pid_;
  boost::asio::io_service ios_;
  settings st_;

  shared_descriptor tap_;

  boost::shared_ptr<listen_mode> listen_mode_;
};

}

#endif // SECRET_PASSAGE_SERVICE_HPP
