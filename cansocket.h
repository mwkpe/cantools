#ifndef CAN_SOCKET_H
#define CAN_SOCKET_H


#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <string>
#include <array>
#include <stdexcept>


namespace can
{


class Socket_error : public std::runtime_error
{
public:
  Socket_error(const std::string& s) : std::runtime_error{s} {}
  Socket_error(const char* s) : std::runtime_error{s} {}
};


class Socket
{
public:
  Socket() : fd_{-1} {}
  ~Socket() { close(); }

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;
  Socket(Socket&&) = delete;
  Socket& operator=(Socket&&) = delete;

  void open(const std::string& device);
  void close();

  void bind();
  void set_receive_timeout(time_t timeout);
  void set_socket_timestamp(bool enable);

  int transmit(const can_frame* frame);
  int receive(can_frame* frame);
  int receive(can_frame* frame, std::uint64_t* time);

private:
  void reset();

  int fd_;
  sockaddr_can addr_;
  iovec iov_;
  msghdr msg_;
  std::array<uint8_t, CMSG_SPACE(sizeof(timeval))> cmsg_buffer;  // Receive time
};


}  // namespace can


#endif  // CAN_SOCKET_H
