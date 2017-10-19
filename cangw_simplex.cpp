/* A small command line program for routing frames from a CAN socket to an UDP socket
 */


#include <cstdint>
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>

#include "cxxopts.hpp"
#include "cansocket.h"
#include "udpsocket.h"
#include "priority.h"


void route_frames(std::atomic<bool>& stop, std::string device, std::string ip, std::uint16_t port,
    bool timestamp)
{
  can::Socket can_socket;
  udp::Socket udp_socket;
  try {
    can_socket.open(device);
    can_socket.bind();
    can_socket.set_receive_timeout(3);
    if (timestamp)
      can_socket.set_socket_timestamp(true);
    udp_socket.open(ip, port);
  }
  catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return;
  }

  if (timestamp) {
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
  else {
    can_frame frame;
    while (!stop.load()) {
      if (can_socket.receive(&frame) == sizeof(can_frame)) {
        udp_socket.transmit(&frame);
      }
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
    cxxopts::Options options{"cangw", "CAN to UDP simplex gateway"};
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

  try {
    std::tie(can_device, remote_ip, data_port, realtime, timestamp) = parse_args(argc, argv);
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Error parsing command line options:\n" << e.what() << std::endl;
    return 1;
  }

  std::cout << "Routing frames from " << can_device << " to " << remote_ip << ":"
      << data_port << "\nPress enter to stop..." << std::endl;

  std::atomic<bool> stop{false};
  std::thread gateway{&route_frames, std::ref(stop), can_device, remote_ip, data_port, timestamp};

  if (realtime) {
    if (priority::set_realtime(gateway.native_handle()))
      std::cout << "Gateway thread set to realtime scheduling policy\n";
    else
      std::cout << "Warning: Could not set scheduling policy, forgot sudo?\n";
  }

  std::cin.ignore();  // Wait in main thread

  std::cout << "Stopping gateway..." << std::endl;
  stop.store(true);
  gateway.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
