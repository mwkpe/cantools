#ifndef OPEN_SOCKET_H_
#define OPEN_SOCKET_H_


// RAII wrapper for the socket


#include <unistd.h>


struct open_socket
{
  explicit open_socket(int id) : id{id} {};
  ~open_socket() { if (id >= 0) close(id); }

  open_socket(const open_socket&) = delete;
  open_socket& operator=(const open_socket&) = delete;
  open_socket(open_socket&&) = delete;
  open_socket& operator=(open_socket&&) = delete;
  
  int id;
};


#endif
