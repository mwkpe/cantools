#ifndef CAN_SOCKET_H
#define CAN_SOCKET_H


#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <stdexcept>
#include <string>


namespace can
{


class socket_error : public std::runtime_error
{
public:
  socket_error(const std::string& s) : std::runtime_error{s} {}
  socket_error(const char* s) : std::runtime_error{s} {}
};


class socket
{
public:
  socket() : fd_{-1} {}
  ~socket() { close(); }

  socket(const socket&) = delete;
  socket& operator=(const socket&) = delete;
  socket(socket&&) = delete;
  socket& operator=(socket&&) = delete;

  void open(const std::string& device);
  void close();

  void bind();
  void set_receive_timeout(time_t timeout);

  int transmit(const can_frame* frame);
  int receive(can_frame* frame);

private:
  void reset();

  int fd_;
  sockaddr_can addr_;
  iovec iov_;
  msghdr msg_;
};


}  // namespace can


#endif
