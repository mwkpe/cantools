#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H


#include <sys/socket.h>
#include <netinet/in.h>

#include <cstdint>
#include <string>
#include <stdexcept>

struct can_frame;


namespace udp
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

  void open(const std::string& ip, std::uint16_t port);
  void close();

  void bind();
  void set_receive_timeout(time_t timeout);

  int transmit(const can_frame* frame);
  int receive(can_frame* frame);

private:
  void reset();

  int fd_;
  sockaddr_in addr_;
};


}  // namespace can


#endif  // UDP_SOCKET_H
