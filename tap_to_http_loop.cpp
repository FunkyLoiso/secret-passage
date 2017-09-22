#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include "tap_to_http_loop.hpp"
#include "logging.hpp"

namespace asio = boost::asio;

namespace sp
{

namespace {
static const char CRLF[] = "\r\n";
}

/*\
 *  class tap_to_http_loop::private_t
\*/
class tap_to_http_loop::private_t {
public:
  private_t(boost::shared_ptr<socket>, asio::posix::stream_descriptor& tap, loop_stop_handler lsh);
  void start();

private:
  void async_read_tap();
  void handle_read_tap(const boost::system::error_code& ec, std::size_t tr);

  void async_write_http_chunk();
  void handle_write_http_chunk(const boost::system::error_code& ec, std::size_t tr);

  asio::posix::stream_descriptor& tap_;
  boost::shared_ptr<socket> socket_;
  loop_stop_handler loop_stop_;

  asio::streambuf chunk_size_buf_;
  asio::streambuf tap_buf_;  // tap -> tap_buf_ -> (converted to chunk) -> http
};

tap_to_http_loop::private_t::private_t(boost::shared_ptr<socket> socket, asio::posix::stream_descriptor& tap, loop_stop_handler lsh)
  : socket_(socket), tap_(tap), loop_stop_(lsh)
{}

void tap_to_http_loop::private_t::start() {
  // 'clear' buffer input
  chunk_size_buf_.consume(chunk_size_buf_.size());
  tap_buf_.consume(tap_buf_.size());

  async_read_tap();
}

void tap_to_http_loop::private_t::async_read_tap() {
  tap_.async_read_some(
    tap_buf_.prepare(512),
    boost::bind(
      &private_t::handle_read_tap,
        this,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
    )
  );

//  std::ostream str(&tap_buf_);
//  static int count = 0;
//  if(++count < 5) {
//    str << count;
//    boost::system::error_code dummy;
//    handle_read_tap(dummy, 1);
//  }
}

void tap_to_http_loop::private_t::handle_read_tap(const boost::system::error_code& ec, std::size_t tr) {
  static const char func[] = "tap_to_http_loop::handle_read_tap";
  LOG_TRACE << bf("%s: %d bytes read") % func % tr;
  if(asio::error::operation_aborted == ec) {
    return;
  }

  if(ec) {
    LOG_ERROR << (bf("%s: reading from tap failed: %s") % func % ec.message());
    loop_stop_(loop_stop_reason::tap_read_error);
    return;
  }
  // we have some packets
  tap_buf_.commit(tr); // @TODO: commit whole number of packets
  async_write_http_chunk();
}

void tap_to_http_loop::private_t::async_write_http_chunk() {
  static const char func[] = "tap_to_http_loop::async_write_http_chunk";
  // prepare chunk size buffer
  std::ostream sstr(&chunk_size_buf_);
  sstr << std::hex << tap_buf_.size() << CRLF;
  auto last_buf = asio::const_buffer(CRLF, 2);

  std::vector<asio::const_buffer> buffers;
  buffers.push_back(chunk_size_buf_.data());
  buffers.push_back(tap_buf_.data());
  buffers.push_back(last_buf);

  LOG_TRACE << bf("%s: writing http chunk (%d bytes):\n%s%s%s")
    % func
    % asio::buffer_size(buffers)
    % std::string(asio::buffer_cast<const char*>(chunk_size_buf_.data()), chunk_size_buf_.size())
    % std::string(asio::buffer_cast<const char*>(tap_buf_.data()), tap_buf_.size())
    % CRLF;
  socket_->async_write_some(
    buffers,
    boost::bind(
      &private_t::handle_write_http_chunk,
        this,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
    )
  );
}

void tap_to_http_loop::private_t::handle_write_http_chunk(const boost::system::error_code& ec, std::size_t tr) {
  static const char func[] = "tap_to_http_loop::handle_write_http_chunk";
  if(asio::error::operation_aborted == ec) {
    return;
  }

  if(ec) {
    LOG_ERROR << bf("%s: failed to write http chunk, setting reconnect timer: %s")
      % func % ec.message();
    loop_stop_(loop_stop_reason::socket_write_error);
    return;
  }
  chunk_size_buf_.consume(chunk_size_buf_.size());
  tap_buf_.consume(tap_buf_.size());
  async_read_tap();
}

/*\
 *  class tap_to_http_loop
\*/
tap_to_http_loop::tap_to_http_loop(boost::shared_ptr<socket> s, asio::posix::stream_descriptor& tap, loop_stop_handler lsh)
  : p(new private_t(s, tap, lsh))
{}

void tap_to_http_loop::start() {
  p->start();
}

}
