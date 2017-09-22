#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include "listen_mode.hpp"
#include "logging.hpp"
#include "normal_socket.hpp"
#include "secure_socket.hpp"
#include "http_parser.hpp"
#include "loop_stop.hpp"
#include "http_to_tap_loop.hpp"
#include "tap_to_http_loop.hpp"

namespace asio = boost::asio;

namespace sp
{

namespace {
static const char CRLF[] = "\r\n";
}

/*\
 *  class listen_mode::private_t
\*/
class listen_mode::private_t {
public:
  private_t(boost::asio::io_service& ios, const settings& st, shared_descriptor tap);
  ~private_t();
  void setup();

private:
  void close_and_listen();
  void async_accept();
  void handle_accept(const boost::system::error_code& ec);

  bool handle_headers_complete(const http_parser* parser);

  void async_write_http_headers();
  void handle_write_http_headers(const boost::system::error_code& ec, std::size_t tr);

  void handle_loop_stop(loop_stop_reason::code_t code);


  boost::asio::io_service& ios_;
  const settings& st_;
  asio::posix::stream_descriptor tap_;

  socket::acceptor acceptor_;
  socket::acceptor::endpoint_type remote_ep_;
  boost::shared_ptr<socket> socket_;
  boost::shared_ptr<http_to_tap_loop> htt_loop_;
  boost::shared_ptr<tap_to_http_loop> tth_loop_;

  asio::streambuf http_headers_buf_;
};

listen_mode::private_t::private_t(boost::asio::io_service& ios, const settings& st, shared_descriptor tap)
  : ios_(ios), st_(st), tap_(ios_, tap->get())
  , acceptor_(ios_)
{
  socket_ = boost::make_shared<normal_socket>(ios_); // @TODO: read tls option from settings
  htt_loop_ = boost::make_shared<http_to_tap_loop>(
    socket_,
    tap_,
    boost::bind(
      &private_t::handle_headers_complete,
        this,
        boost::placeholders::_1
    ),
    boost::bind(
      &private_t::handle_loop_stop,
        this,
        boost::placeholders::_1
    )
  );

  tth_loop_ = boost::make_shared<tap_to_http_loop>(
    socket_,
    tap_,
    boost::bind(
      &private_t::handle_loop_stop,
        this,
        boost::placeholders::_1
    )
  );
}

listen_mode::private_t::~private_t() {
  boost::system::error_code close_ec;
  socket_->shutdown(close_ec);
  if(close_ec) {
    LOG_WARNING << bf("listen_mode dtor: error while shutting socket down after accept failure: %s")
      % close_ec.message();
  }
}

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
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
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
  async_accept();
}

void listen_mode::private_t::close_and_listen() {
  static const char func[] = "listen_mode::close_and_listen";
  boost::system::error_code close_ec;
  socket_->cancel(close_ec);
  tap_.cancel(close_ec);
  socket_->shutdown(close_ec);
  if(close_ec) {
    LOG_WARNING << bf("%s: error while shutting socket down after accept failure: %s")
      % func % close_ec.message();
  }
  socket_->close(close_ec);
  if(close_ec) {
    LOG_WARNING << bf("%s: error while closing socket after accept failure: %s")
      % func % close_ec.message();
  }
  acceptor_.cancel();
  // 'clear' buffer input
  http_headers_buf_.consume(http_headers_buf_.size());
  async_accept();
  return;
}

void listen_mode::private_t::async_accept() {
  socket_->async_accept(
    acceptor_,
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
    close_and_listen();
    return;
  }

  LOG_INFO << bf("%s: accepted connection from '%s:%d', awaiting request..")
    % func % remote_ep_.address().to_string() % remote_ep_.port();

  htt_loop_->start();
}

bool listen_mode::private_t::handle_headers_complete(const http_parser* parser) {
  static const char func[] = "listen_mode::handle_headers_complete";
  LOG_DEBUG << bf("%s: Headers complete. Url is '%s', headers are:") % func % parser->url().full;
  for(auto i = parser->headers().begin(); i != parser->headers().end(); ++i) {
    LOG_DEBUG << bf("\t%s: %s") % i->first % i->second;
  }
  async_write_http_headers();
  return true;
}

void listen_mode::private_t::async_write_http_headers() {
  LOG_TRACE << __PRETTY_FUNCTION__;
  std::ostream str(&http_headers_buf_);
  str << "HTTP/1.1 200 OK" << CRLF;
  str << "Transfer-Encoding: chunked" << CRLF;
  str << "Content-Type: application/octet-stream" << CRLF;
  str << CRLF;

  socket_->async_write_some(
    http_headers_buf_.data(),
    boost::bind(
      &private_t::handle_write_http_headers,
        this,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
    )
  );
}

void listen_mode::private_t::handle_write_http_headers(const boost::system::error_code& ec, std::size_t tr) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  static const char func[] = "listen_mode::handle_write_http_headers";
  if(asio::error::operation_aborted == ec) {
    return;
  }
  if(ec) {
    LOG_WARNING << bf("%s: failed to write headers, setting reconnect timer: %s")
      % func % ec.message();
    close_and_listen();
    return;
  }

  http_headers_buf_.consume(tr);
  tth_loop_->start();

}

void listen_mode::private_t::handle_loop_stop(loop_stop_reason::code_t code) {
  close_and_listen();
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
