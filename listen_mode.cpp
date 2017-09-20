#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include "listen_mode.hpp"
#include "logging.hpp"
#include "normal_socket.hpp"
#include "secure_socket.hpp"
#include "http_parser.hpp"

namespace asio = boost::asio;

namespace sp
{

/*\
 *  class listen_mode::private_t
\*/
class listen_mode::private_t: public http_parser_handler {
public:
  private_t(boost::asio::io_service& ios, const settings& st, shared_descriptor tap);
  ~private_t();
  void setup();

private:
  void close_and_listen();
  void async_accept();
  void handle_accept(const boost::system::error_code& ec);
  void async_read();
  void handle_read(const boost::system::error_code& ec, std::size_t tr);

  // http_parser_handler interface
  virtual void handle_headers_complete(const http_parser* parser);
  virtual bool handle_body(const http_parser* parser, const char* data, std::size_t size);


  boost::asio::io_service& ios_;
  const settings& st_;
  shared_descriptor tap_;

  socket::acceptor acceptor_;
  socket::acceptor::endpoint_type remote_ep_;
  boost::shared_ptr<socket> socket_;

  http_parser parser_;

  struct state {
    enum code_t {
      no_connection,
      accepting_header,
      accepting_data
    };
  };
  state::code_t state_;

  asio::streambuf buf_;
};

listen_mode::private_t::private_t(boost::asio::io_service& ios, const settings& st, shared_descriptor tap)
  : ios_(ios), st_(st), tap_(tap)
  , acceptor_(ios_)
  , state_(state::no_connection)
  , parser_(this)
{
  socket_ = boost::make_shared<normal_socket>(ios_); // @TODO: read tls option from settings
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
  parser_.reset();
  // 'clear' buffer input
  buf_.consume(buf_.size());
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
  state_ = state::accepting_header;
  async_read();
}

void listen_mode::private_t::async_read() {
  socket_->async_read_some(
    buf_.prepare(512),
    boost::bind(
      &private_t::handle_read,
        this,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
    )
  );
}

void listen_mode::private_t::handle_read(const boost::system::error_code& ec, std::size_t tr) {
  static const char func[] = "listen_mode::handle_read";
  if(asio::error::operation_aborted == ec) {
    LOG_TRACE << func << ": operation aborted";
    return;
  }

  // @TODO: handle close errors differently?
  if(ec) {
    LOG_WARNING << bf("%s: read operation error, closing socket and accepting again: %s")
      % func % ec.message();
    close_and_listen();
    return;
  }

  // actually some data!
  buf_.commit(tr);
  auto data = asio::buffer_cast<const char*>(buf_.data());
  auto size = buf_.size();
  LOG_TRACE << bf("%s: received request data (%d bytes):\n%s")
    % func % tr % std::string(data, size);
  auto consumed = parser_.notify(data, size);
  LOG_DEBUG << bf("%s: parser consumed %d/%d data") % func % consumed % size;
  if(parser_.failed()) {
    LOG_ERROR << bf("%s: request parsing error, resetting connection: %s") % func % parser_.error();
    close_and_listen();
    return;
  }
  buf_.consume(consumed);

  async_read();
}

void listen_mode::private_t::handle_headers_complete(const http_parser* parser) {
  static const char func[] = "listen_mode::handle_headers_complete";
  LOG_DEBUG << bf("%s: Headers complete. Url is '%s', headers are:") % func % parser->url().full;
  for(auto i = parser->headers().begin(); i != parser->headers().end(); ++i) {
    LOG_DEBUG << bf("\t%s: %s") % i->first % i->second;
  }
}

bool listen_mode::private_t::handle_body(const http_parser* parser, const char* data, std::size_t size) {
  static const char func[] = "listen_mode::handle_body";
  LOG_TRACE << bf("%s: body part (%d bytes):\n%s")
    % func % size % std::string(data, size);
  for(std::size_t total = 0; total < size;) {
    auto written = write(tap_->get(), data + total, size - total);
    if(-1 == written) {
      LOG_ERROR << bf("Writing to tap interface failed, resetting connection: %s") % strerror(errno);
      close_and_listen(); //@TODO: maybe we should not reset the parser before returning..
      return false;
    }
    total += written;
  }
  return true;
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
