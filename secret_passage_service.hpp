#ifndef SECRET_PASSAGE_SERVICE_HPP
#define SECRET_PASSAGE_SERVICE_HPP

#include <boost/asio/io_service.hpp>
#include "settings.hpp"

namespace sp
{

class secret_passage_service
{
public:
  secret_passage_service(settings st);
  void run();

private:
  boost::asio::io_service ios_;
  settings st_;
};

}

#endif // SECRET_PASSAGE_SERVICE_HPP
