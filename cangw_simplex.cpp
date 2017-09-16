/* A small command line program for routing frames from a CAN socket to an UDP socket
 */


#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>

#include "cxxopts.hpp"
#include "cansocket.h"
#include "udpsocket.h"


void route_frames(std::atomic<bool>& stop, std::string device, std::string ip, std::uint16_t port)
{
  can::socket can_socket;
  udp::socket udp_socket;
  try {
    can_socket.open(device);
    can_socket.bind();
    can_socket.set_receive_timeout(3);
    udp_socket.open(ip, port);
  }
  catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return;
  }

  can_frame frame;
  while (!stop.load()) {
    if (can_socket.receive(&frame) == sizeof(can_frame)) {
      udp_socket.transmit(&frame);
    }
  }
}


std::tuple<std::string, std::string, std::uint16_t> parse_args(int argc, char **argv)
{
  std::string can_device;
  std::string remote_ip;
  std::uint16_t remote_port;

  try {
    cxxopts::Options options{"cangw", "CAN to UDP router"};
    options.add_options()
      ("ip", "Remote device IP", cxxopts::value<std::string>(remote_ip))
      ("port", "UDP port", cxxopts::value<std::uint16_t>(remote_port))
      ("device", "CAN device name", cxxopts::value<std::string>(can_device)->default_value("can0"))
    ;
    options.parse(argc, argv);

    if (options.count("ip") == 0) {
      throw std::runtime_error{"Remote IP must be specified, use the --ip option"};
    }
    if (options.count("port") == 0) {
      throw std::runtime_error{"UDP port must be specified, use the --port option"};
    }

    return std::make_tuple(std::move(can_device), std::move(remote_ip), remote_port);
  }
  catch (const cxxopts::OptionException& e) {
    throw std::runtime_error{e.what()};
  }
}


int main(int argc, char **argv)
{
  std::string can_device;
  std::string remote_ip;
  std::uint16_t remote_port;

  try {
    std::tie(can_device, remote_ip, remote_port) = parse_args(argc, argv);
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Error parsing command line options:\n" << e.what() << std::endl;
    return 1;
  }

  std::cout << "Routing frames from " << can_device << " to " << remote_ip << ":"
      << remote_port << "\nPress enter to stop..." << std::endl;

  std::atomic<bool> stop{false};
  std::thread gateway{&route_frames, std::ref(stop), can_device, remote_ip, remote_port};
  std::cin.ignore();  // Wait in main thread

  std::cout << "Stopping gateway..." << std::endl;
  stop.store(true);
  gateway.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
