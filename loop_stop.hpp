#ifndef LOOP_STOP_HPP
#define LOOP_STOP_HPP

namespace sp
{

namespace loop_stop_reason
{
enum code_t {
  socket_read_error = 1,
  socket_write_error,
  request_prasing_error,
  tap_read_error,
  tap_write_error,
};
}


typedef boost::function<void(loop_stop_reason::code_t)> loop_stop_handler;


}

#endif // LOOP_STOP_HPP
