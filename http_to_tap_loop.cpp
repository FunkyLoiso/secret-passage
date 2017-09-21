#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include "http_to_tap_loop.hpp"
#include "logging.hpp"
#include "http_parser.hpp"

namespace asio = boost::asio;

namespace sp
{

class http_to_tap_loop::private_t: public http_parser_handler {
public:
  private_t(boost::shared_ptr<socket>, asio::posix::stream_descriptor& tap, headers_complete_handler hch, loop_stop_handler lsh);
  void start();

private:
  void async_read_http();
  void handle_read_http(const boost::system::error_code& ec, std::size_t tr);

  void async_write_tap();
  void handle_write_tap(const boost::system::error_code& ec, std::size_t tr);

  // http_parser_handler interface
  virtual bool handle_headers_complete(const http_parser* parser);
  virtual bool handle_body(const http_parser* parser, const char* data, std::size_t size);

  asio::posix::stream_descriptor& tap_;
  boost::shared_ptr<socket> socket_;
  headers_complete_handler headers_complete_;
  loop_stop_handler loop_stop_;

  http_parser parser_;

  asio::streambuf http_buf_;  // http -> http_buf_ -> parser_ -> tap
  std::vector<asio::const_buffer> send_buffers_;
  // for now we just transfer
//  asio::streambuf tap_buf_;
};

http_to_tap_loop::private_t::private_t(boost::shared_ptr<socket> socket, asio::posix::stream_descriptor& tap, headers_complete_handler hch, loop_stop_handler lsh)
  : socket_(socket), tap_(tap), headers_complete_(hch), loop_stop_(lsh)
  , parser_(this)
{}

void http_to_tap_loop::private_t::start() {
  parser_.reset();
  send_buffers_.clear();
  // 'clear' buffer input
  http_buf_.consume(http_buf_.size());
//  tap_buf_.consume(tap_buf_.size());

  async_read_http();
}

void http_to_tap_loop::private_t::async_read_http() {
  socket_->async_read_some(
    http_buf_.prepare(512),
    boost::bind(
      &private_t::handle_read_http,
        this,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
    )
  );
}

void http_to_tap_loop::private_t::handle_read_http(const boost::system::error_code& ec, std::size_t tr) {
  static const char func[] = "http_to_tap_loop::handle_read_http";
  if(asio::error::operation_aborted == ec) {
    LOG_TRACE << func << ": operation aborted";
    return;
  }

  // @TODO: handle close errors differently?
  if(ec) {
    LOG_WARNING << bf("%s: failed to read from socket: %s")
      % func % ec.message();
    loop_stop_(loop_stop_reason::socket_read_error);
    return;
  }

  // actually some data!
  http_buf_.commit(tr);
  auto data = asio::buffer_cast<const char*>(http_buf_.data());
  auto size = http_buf_.size();
  LOG_TRACE << bf("%s: received request data (%d bytes):\n%s")
    % func % tr % std::string(data, size);
  auto consumed = parser_.notify(data, size);
  LOG_DEBUG << bf("%s: parser consumed %d/%d data") % func % consumed % size;
  if(parser_.failed()) {
    LOG_ERROR << bf("%s: request parsing error, resetting connection: %s") % func % parser_.error();
    loop_stop_(loop_stop_reason::request_prasing_error);
    return;
  }

  if(!send_buffers_.empty()) {
    async_write_tap();
  }
  else {
    http_buf_.consume(http_buf_.size());
    async_read_http();
  }
}

void http_to_tap_loop::private_t::async_write_tap() {
  tap_.async_write_some(
    send_buffers_,
    boost::bind(
      &private_t::handle_write_tap,
        this,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
    )
  );
}

void http_to_tap_loop::private_t::handle_write_tap(const boost::system::error_code& ec, std::size_t tr) {
  static const char func[] = "http_to_tap_loop::handle_write_tap";
  if(asio::error::operation_aborted == ec) {
    return;
  }

  if(ec) {
    LOG_WARNING << bf("%s: failed to write to tap: %s")
      % func % ec.message();
    loop_stop_(loop_stop_reason::tap_write_error);
    return;
  }

  send_buffers_.clear();
  http_buf_.consume(http_buf_.size());
  async_read_http();
}

bool http_to_tap_loop::private_t::handle_headers_complete(const http_parser* parser) {
  static const char func[] = "http_to_tap_loop::handle_headers_complete";
  LOG_DEBUG << bf("%s: Headers complete. Url is '%s', headers are:") % func % parser->url().full;
  for(auto i = parser->headers().begin(); i != parser->headers().end(); ++i) {
    LOG_DEBUG << bf("\t%s: %s") % i->first % i->second;
  }
  return headers_complete_(parser);
}

bool http_to_tap_loop::private_t::handle_body(const http_parser* parser, const char* data, std::size_t size) {
  static const char func[] = "http_to_tap_loop::handle_body";
  LOG_TRACE << bf("%s: body part (%d bytes):\n%s")
    % func % size % std::string(data, size);

  send_buffers_.push_back(asio::const_buffer(data, size));
}

}
