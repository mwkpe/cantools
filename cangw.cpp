/* A small command line program for routing frames between a CAN socket and an UDP socket
 */


#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>

#include "cxxopts.hpp"
#include "cansocket.h"
#include "udpsocket.h"
#include "priority.h"


void route_to_udp(can::Socket& can_socket, udp::Socket& udp_socket, std::atomic<bool>& stop)
{
  can_frame frame;
  while (!stop.load()) {
    if (can_socket.receive(&frame) == sizeof(can_frame)) {
      udp_socket.transmit(&frame);
    }
  }
}


void route_to_udp_with_timestamp(can::Socket& can_socket, udp::Socket& udp_socket,
    std::atomic<bool>& stop)
{
  // Pass-through of original receive timestamp for more accurate timing information of frames
  std::vector<std::uint8_t> buffer(sizeof(std::uint64_t) + sizeof(can_frame));
  auto* time = reinterpret_cast<std::uint64_t*>(buffer.data());
  auto* frame = reinterpret_cast<can_frame*>(buffer.data() + sizeof(std::uint64_t));
  while (!stop.load()) {
    // Ancillary data (timestamp) is not part of socket payload
    if (can_socket.receive(frame, time) == sizeof(can_frame)) {
      udp_socket.transmit(buffer);
    }
  }
}


void route_to_can(can::Socket& can_socket, udp::Socket& udp_socket, std::atomic<bool>& stop)
{
  can_frame frame;
  while (!stop.load()) {
    if (udp_socket.receive(&frame) == sizeof(can_frame)) {
      can_socket.transmit(&frame);
    }
  }
}


std::tuple<std::string, std::string, std::uint16_t, bool, bool> parse_args(int argc, char** argv)
{
  bool realtime = false;
  bool timestamp = false;
  std::string remote_ip;
  std::uint16_t data_port;
  std::string can_device;

  try {
    cxxopts::Options options{"cangw", "CAN to UDP duplex gateway"};
    options.add_options()
      ("r,realtime", "Enable realtime scheduling policy", cxxopts::value<bool>(realtime))
      ("t,timestamp", "Prefix UDP packets with timestamp", cxxopts::value<bool>(timestamp))
      ("i,ip", "Remote device IP", cxxopts::value<std::string>(remote_ip))
      ("p,port", "UDP data port", cxxopts::value<std::uint16_t>(data_port))
      ("d,device", "CAN device name", cxxopts::value<std::string>(can_device)->default_value("can0"))
    ;
    options.parse(argc, argv);

    if (options.count("ip") == 0) {
      throw std::runtime_error{"Remote IP must be specified, use the -i or --ip option"};
    }
    if (options.count("port") == 0) {
      throw std::runtime_error{"UDP port must be specified, use the -p or --port option"};
    }

    return std::make_tuple(std::move(can_device), std::move(remote_ip), data_port, realtime, timestamp);
  }
  catch (const cxxopts::OptionException& e) {
    throw std::runtime_error{e.what()};
  }
}


int main(int argc, char** argv)
{
  bool realtime;
  bool timestamp; // Pass original CAN receive timestamp to remote device
  std::string remote_ip;
  std::uint16_t data_port;
  std::string can_device;
  can::Socket can_socket;
  udp::Socket udp_socket;

  try {
    std::tie(can_device, remote_ip, data_port, realtime, timestamp) = parse_args(argc, argv);
    can_socket.open(can_device);
    can_socket.bind();
    can_socket.set_receive_timeout(3);
    if (timestamp)
      can_socket.set_socket_timestamp(true);
    udp_socket.open(remote_ip, data_port);  // Transmit to remote device
    udp_socket.bind("0.0.0.0", data_port);  // Receive from remote device
    udp_socket.set_receive_timeout(3);
  }
  catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  std::cout << "Routing frames between " << can_device << " and " << remote_ip << ":"
      << data_port << "\nPress enter to stop..." << std::endl;

  std::atomic<bool> stop{false};
  std::thread can_to_udp{};
  if (timestamp) {
    can_to_udp = std::thread{&route_to_udp_with_timestamp, std::ref(can_socket),
        std::ref(udp_socket), std::ref(stop)};
  }
  else {
    can_to_udp = std::thread{&route_to_udp, std::ref(can_socket), std::ref(udp_socket), std::ref(stop)};
  }
  std::thread udp_to_can{&route_to_can, std::ref(can_socket), std::ref(udp_socket), std::ref(stop)};

  if (realtime) {
    if (priority::set_realtime(can_to_udp.native_handle()) &&
        priority::set_realtime(udp_to_can.native_handle()))
      std::cout << "Gateway threads set to realtime scheduling policy\n";
    else
      std::cout << "Warning: Could not set scheduling policy, forgot sudo?\n";
  }

  std::cin.ignore();  // Wait in main thread

  std::cout << "Stopping gateway..." << std::endl;
  stop.store(true);
  can_to_udp.join();
  udp_to_can.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
