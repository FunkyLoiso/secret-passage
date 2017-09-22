#include <vector>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include "connect_mode.hpp"
#include "normal_socket.hpp"
#include "secure_socket.hpp"
#include "http_parser.hpp"
#include "http_to_tap_loop.hpp"
#include "tap_to_http_loop.hpp"
#include "logging.hpp"

namespace asio = boost::asio;

namespace sp
{

namespace
{
  static const char CRLF[] = "\r\n";
}

/*\
 *  class connect_mode::private_t
\*/
class connect_mode::private_t {
public:
  private_t(boost::asio::io_service& ios, const settings& st, shared_descriptor tap);
  ~private_t();
  void setup();

private:
  typedef asio::ip::tcp::resolver resolver_t;

  void close_and_reconnect();
  void set_reconnect_timer();
  void async_reconnect();
  void handle_resolve(const boost::system::error_code& ec, resolver_t::iterator i);
  void async_connect(resolver_t::iterator i);
  void handle_connect(const boost::system::error_code& ec, resolver_t::iterator i);

  void async_write_http_headers();
  void handle_write_http_headers(const boost::system::error_code& ec, std::size_t tr);

  bool handle_headers_complete(const http_parser* parser);
  void handle_htt_loop_stop(loop_stop_reason::code_t code);

  asio::io_service& ios_;
  const settings& st_;
  asio::posix::stream_descriptor tap_;

  resolver_t resolver_;
  asio::steady_timer reconnect_timer_;

  std::string host_, port_;
  boost::shared_ptr<socket> socket_;
  boost::shared_ptr<http_to_tap_loop> htt_loop_;
  boost::shared_ptr<tap_to_http_loop> tth_loop_;

  asio::streambuf http_heades_buf_;
};

connect_mode::private_t::private_t(asio::io_service& ios, const settings& st, shared_descriptor tap)
  : ios_(ios), st_(st), tap_(ios_, tap->get())
  , resolver_(ios_)
  , reconnect_timer_(ios_)
{
  socket_ = boost::make_shared<normal_socket>(ios_); //@TODO: read tls option from settings
  htt_loop_ = boost::make_shared<http_to_tap_loop>(
    socket_,
    tap_,
    boost::bind(
      &private_t::handle_headers_complete,
        this,
        boost::placeholders::_1
    ),
    boost::bind(
      &private_t::handle_htt_loop_stop,
        this,
        boost::placeholders::_1
    )
  );
  tth_loop_ = boost::make_shared<tap_to_http_loop>(
    socket_,
    tap_,
    boost::bind(
      &private_t::handle_htt_loop_stop,
        this,
        boost::placeholders::_1
    )
  );
}

connect_mode::private_t::~private_t() {
  boost::system::error_code close_ec;
  socket_->shutdown(close_ec);
  if(close_ec) {
    LOG_WARNING << bf("connect_mode dtor: error while shutting socket down after accept failure: %s")
      % close_ec.message();
  }

}

void connect_mode::private_t::setup() {
  // split host and port
  auto colon = st_.address.find(':');
  if(std::string::npos == colon) {
    host_ = st_.address;
    port_ = "443";
  }
  else {
    host_ = st_.address.substr(0, colon);
    port_ = st_.address.substr(colon+1);
  }

  async_reconnect();
}

void connect_mode::private_t::close_and_reconnect() {
  static const char func[] = "connect_mode::close_and_reconnect";
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
  resolver_.cancel();
  // 'clear' buffers
  http_heades_buf_.consume(http_heades_buf_.size());
  async_reconnect();
  return;
}

void connect_mode::private_t::set_reconnect_timer() {
  static const char func[] = "connect_mode::set_reconnect_timer";
  LOG_TRACE << bf("%s: setting reconnect time for %d ms")
    % func % std::chrono::duration_cast<std::chrono::milliseconds>(st_.reconnect_interval).count();
  reconnect_timer_.expires_from_now(st_.reconnect_interval);
  reconnect_timer_.async_wait(boost::bind(
    &private_t::close_and_reconnect,
      this
  ));
}

void connect_mode::private_t::async_reconnect() {
  static const char func[] = "connect_mode::async_reconnect";
  LOG_DEBUG << bf("%s: resolving host '%s', port '%s'") % func % host_ % port_;

  resolver_t::query q(host_, port_);
  resolver_.async_resolve(q, boost::bind(
    &private_t::handle_resolve,
      this,
        asio::placeholders::error,
        asio::placeholders::iterator
  ));
}

void connect_mode::private_t::handle_resolve(const boost::system::error_code& ec, resolver_t::iterator i) {
  static const char func[] = "connect_mode::handle_resolve";
  if(asio::error::operation_aborted == ec) {
    return;
  }

  if(ec) {
    LOG_ERROR << bf("%s: resolve of '%s:%d' failed, setting reconnect timer: %s")
      % func % host_ % port_ % ec.message();
    set_reconnect_timer();
    return;
  }
  async_connect(i);
}

void connect_mode::private_t::async_connect(resolver_t::iterator i) {
  socket_->async_connect(*i, boost::bind(
    &private_t::handle_connect,
      this,
      asio::placeholders::error,
      i
  ));
}

void connect_mode::private_t::handle_connect(const boost::system::error_code& ec, resolver_t::iterator i) {
  static const char func[] = "connect_mode::handle_connect";
  if(asio::error::operation_aborted == ec) {
    return;
  }

  if(ec) {
    LOG_DEBUG << bf("%s: connection to endpoint '%s:%d' failed, continuing: %s")
      % func % i->endpoint().address().to_string() % i->endpoint().port() % ec.message();
    if(++i != resolver_t::iterator()) {
      async_connect(i);
      return;
    }
    else {
      LOG_ERROR << bf("%s: couldn't establish connection to any of the endpoints, setting reconnect timer")
       % func;
      set_reconnect_timer();
      return;
    }
  }
  // we have connected
  async_write_http_headers();
}

void connect_mode::private_t::async_write_http_headers() {
  LOG_TRACE << __PRETTY_FUNCTION__;
  std::ostream str(&http_heades_buf_);
  str << "POST /tunnel HTTP/1.1" << CRLF;
  str << "Host: " << host_ << ":" << port_ << CRLF;
  str << "User-Agent: secret-passage" << CRLF; //@TODO: add version
  str << "Accept: application/octet-stream" << CRLF;
  str << "Transfer-Encoding: chunked" << CRLF;
  str << "Content-Type: application/octet-stream" << CRLF;
  str << CRLF;

  socket_->async_write_some(
    http_heades_buf_.data(),
    boost::bind(
      &private_t::handle_write_http_headers,
        this,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
    )
  );
}

void connect_mode::private_t::handle_write_http_headers(const boost::system::error_code& ec, std::size_t tr) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  static const char func[] = "connect_mode::handle_write_http_headers";
  if(asio::error::operation_aborted == ec) {
    return;
  }
  if(ec) {
    LOG_WARNING << bf("%s: failed to write headers, setting reconnect timer: %s")
      % func % ec.message();
    set_reconnect_timer();
    return;
  }

  http_heades_buf_.consume(tr);
  htt_loop_->start();
}

bool connect_mode::private_t::handle_headers_complete(const http_parser* parser) {
  static const char func[] = "connect_mode::handle_headers_complete";
  LOG_DEBUG << bf("%s: Headers complete. Status: '%d %s' , headers are:")
    % func % parser->code() % parser->status();
  for(auto i = parser->headers().begin(); i != parser->headers().end(); ++i) {
    LOG_DEBUG << bf("\t%s: %s") % i->first % i->second;
  }

  //@TODO: check status
  tth_loop_->start();
  return true;
}

void connect_mode::private_t::handle_htt_loop_stop(loop_stop_reason::code_t code) {
  boost::system::error_code dummy_ec;
  socket_->cancel(dummy_ec);
  tap_.cancel(dummy_ec);
  set_reconnect_timer();
}

/*\
 *  class connect_mode
\*/
connect_mode::connect_mode(asio::io_service& ios, const settings& st, shared_descriptor tap) {
  p = boost::make_shared<private_t>(ios, st, tap);
}

void connect_mode::setup() {
  p->setup();
}

}
