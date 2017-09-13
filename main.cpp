#include <iostream>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>

namespace asio = boost::asio;
namespace chrono = std::chrono;

asio::io_service ios;
asio::steady_timer t(ios);
auto interval = chrono::milliseconds(500);

void handler(const boost::system::error_code& ec) {
  std::cout << "Hi from timer" << std::endl;
  static int n = 0;
  if(++n > 5) return;

  t.expires_from_now(interval);
  t.async_wait(boost::bind(
    handler,
      asio::placeholders::error
  ));
}

int main(int argc, char *argv[]) {
  t.expires_from_now(interval);
  t.async_wait(boost::bind(
    handler,
      asio::placeholders::error
  ));
  ios.run();

  return 0;
}
