#include "udpsocket.h"


#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/can.h>


namespace
{


class Scope_guard
{
public:
  Scope_guard(int fd) : fd_{fd} {}
  ~Scope_guard() { if (fd_ != -1) ::close(fd_); fd_ = -1; }
  Scope_guard(const Scope_guard&) = delete;
  Scope_guard& operator=(const Scope_guard&) = delete;
  void release() { fd_  = -1; }

private:
  int fd_;
};


}  // namespace


void udp::Socket::open(const std::string& ip, std::uint16_t port)
{
  if (fd_ != -1)
    throw Socket_error{"Already open"};

  fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);

  if (fd_ == -1)
    throw Socket_error{"Could not open"};

  Scope_guard guard{fd_};

  addr_.sin_family = AF_INET;

  if (inet_aton(ip.c_str(), &addr_.sin_addr) == 0)
    throw Socket_error{"Error resolving IP address"};

  addr_.sin_port = htons(port);

  guard.release();
}


void udp::Socket::close()
{
  if (fd_ != -1) {
    ::close(fd_);
  }

  reset();
}


void udp::Socket::bind()
{
  if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr_), sizeof(addr_)) < 0)
    throw Socket_error{"Error while binding socket"};
}


void udp::Socket::bind(const std::string& ip, std::uint16_t port)
{
  sockaddr_in addr;
  addr.sin_family = AF_INET;

  if (inet_aton(ip.c_str(), &addr.sin_addr) == 0)
    throw Socket_error{"Error resolving IP address"};

  addr.sin_port = htons(port);

  if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    throw Socket_error{"Error while binding socket"};
}


void udp::Socket::set_receive_timeout(time_t timeout)
{
  if (timeout <= 0)
    throw Socket_error{"Timeout must be larger then 0"};

  timeval tv;
  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  if (setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
    throw Socket_error{"Error setting receive timeout"};
}


int udp::Socket::transmit(const std::vector<std::uint8_t>& data)
{
  return sendto(fd_, data.data(), data.size(), 0, reinterpret_cast<sockaddr*>(&addr_),
      sizeof(addr_));
}


int udp::Socket::transmit(const can_frame* frame)
{
  return sendto(fd_, frame, sizeof(can_frame), 0, reinterpret_cast<sockaddr*>(&addr_),
      sizeof(addr_));
}


int udp::Socket::receive(can_frame* frame)
{
  return recv(fd_, frame, sizeof(can_frame), 0);
}


void udp::Socket::reset()
{
  fd_ = -1;
  addr_ = sockaddr_in{};
}
