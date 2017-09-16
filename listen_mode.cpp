#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include "listen_mode.hpp"
#include "logging.hpp"

namespace asio = boost::asio;

namespace sp
{

/*\
 *  class listen_mode::private_t
\*/
class listen_mode::private_t {
public:
  private_t(boost::asio::io_service& ios, const settings& st, shared_descriptor tap);
  void setup();

private:
  void handle_accept(const boost::system::error_code& ec);

  boost::asio::io_service& ios_;
  const settings& st_;
  shared_descriptor tap_;

  typedef asio::ip::tcp::acceptor acceptor;
  acceptor acceptor_;
  acceptor::endpoint_type remote_ep_;
  typedef asio::ip::tcp::socket socket;
  socket socket_;
};

listen_mode::private_t::private_t(boost::asio::io_service& ios, const settings& st, shared_descriptor tap)
  : ios_(ios), st_(st), tap_(tap)
  , acceptor_(ios_)
  , socket_(ios_)
{}

void listen_mode::private_t::setup() {
  static const char func[] = "listen_mode::setup";
  // split host and port
  std::string host, port;
  auto colon = st_.address.find(':');
  if(std::string::npos == colon) {
    host = st_.address;
    port = "443";
  }
  else {
    host = st_.address.substr(0, colon);
    port = st_.address.substr(colon+1);
  }
  LOG_DEBUG << bf("%s: resolving host '%s', port '%s'") % func % host % port;
  typedef asio::ip::tcp::resolver resolver_t;
  resolver_t::query q(host, port);
  asio::io_service dummy_ios;
  resolver_t resolver(dummy_ios);
  boost::system::error_code ec;
  auto i = resolver.resolve(q, ec);
  if(ec) {
    throw exception(boost::str(bf("Failed to resolve '%s:%s': %s")
      % q.host_name() % q.service_name() % ec.message()
    ));
  }
  auto ep = i->endpoint();
  LOG_DEBUG << bf("%s: resolved to '%s:%d'") % func % ep.address().to_string() % ep.port();
  acceptor_.open(ep.protocol());
//  acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(ep, ec);
  if(ec) {
    throw exception(boost::str(bf("Failed to bind acceptor to endpont '%s:%s': %s")
      % q.host_name() % q.service_name() % ec.message()
    ));
  }
  acceptor_.listen(1, ec); // @TODO: just one connection for now
  if(ec) {
    throw exception(boost::str(bf("Failed to start listening on endpoint '%s:%s': %s")
      % q.host_name() % q.service_name() % ec.message()
    ));
  }
  acceptor_.async_accept(
    socket_,
    remote_ep_,
    boost::bind(
      &private_t::handle_accept,
        this,
        asio::placeholders::error
    )
  );
}

void listen_mode::private_t::handle_accept(const boost::system::error_code& ec) {
  static const char func[] = "listen_mode::handle_accept";
  if(ec == asio::error::operation_aborted) {
    return;
  }

  if(ec) {
    LOG_ERROR << bf("%s: accept opration failed, retrying: %s") % func % ec.message();
    acceptor_.async_accept(
      socket_,
      remote_ep_,
      boost::bind(
        &private_t::handle_accept,
          this,
          asio::placeholders::error
      )
    );
    return;
  }

  LOG_DEBUG << bf("%s: accepted connection from '%s:%d'")
    % func % remote_ep_.address().to_string() % remote_ep_.port();
}

/*\
 *  class listen_mode
\*/
listen_mode::listen_mode(boost::asio::io_service& ios, const settings& st, shared_descriptor tap) {
  p = boost::make_shared<private_t>(ios, st, tap);
}

void listen_mode::setup() {
  p->setup();
}

}
