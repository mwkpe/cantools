#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H


#include <sys/socket.h>
#include <netinet/in.h>

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>


struct can_frame;


namespace udp
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

  void open(const std::string& ip, std::uint16_t port);
  void close();

  void bind();
  void bind(const std::string& ip, std::uint16_t port);
  void set_receive_timeout(time_t timeout);

  int transmit(const std::vector<std::uint8_t>& data);
  int transmit(const can_frame* frame);
  int receive(can_frame* frame);

private:
  void reset();

  int fd_;
  sockaddr_in addr_;
};


}  // namespace udp


#endif  // UDP_SOCKET_H
