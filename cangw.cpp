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


namespace cangw
{


struct Options
{
  bool listen;  // Read frames from CAN bus
  bool send;  // Send frames to CAN bus
  bool realtime;  // Set listen and/or send thread to realtime scheduling policy
  bool timestamp;  // Pass original CAN receive timestamp to remote device
  std::string remote_ip;
  std::uint16_t data_port;
  std::string can_device;
};


}  // namespace cangw


void route_to_udp(can::Socket& can_socket, udp::Socket& udp_socket, std::atomic<bool>& stop,
    bool timestamp)
{
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


void route_to_can(can::Socket& can_socket, udp::Socket& udp_socket, std::atomic<bool>& stop)
{
  can_frame frame;
  while (!stop.load()) {
    if (udp_socket.receive(&frame) == sizeof(can_frame)) {
      can_socket.transmit(&frame);
    }
  }
}


cangw::Options parse_args(int argc, char** argv)
{
  cangw::Options options;
  options.listen = false;
  options.send = false;
  options.realtime = false;
  options.timestamp = false;

  try {
    cxxopts::Options cli_options{"cangw", "CAN to UDP gateway"};
    cli_options.add_options()
      ("l,listen", "Route frames from CAN to UDP", cxxopts::value<bool>(options.listen))
      ("s,send", "Route frames from UDP to CAN", cxxopts::value<bool>(options.send))
      ("r,realtime", "Enable realtime scheduling policy", cxxopts::value<bool>(options.realtime))
      ("t,timestamp", "Prefix UDP payload with timestamp", cxxopts::value<bool>(options.timestamp))
      ("i,ip", "Remote device IP", cxxopts::value<std::string>(options.remote_ip))
      ("p,port", "UDP data port", cxxopts::value<std::uint16_t>(options.data_port))
      ("d,device", "CAN device name", cxxopts::value<std::string>(options.can_device)
          ->default_value("can0"))
    ;
    cli_options.parse(argc, argv);

    if (cli_options.count("listen") + cli_options.count("send") == 0) {
      throw std::runtime_error{"Mode must be specified, use the -l or --listen "
          "and/or -s or --send option"};
    }
    if (cli_options.count("ip") == 0) {
      throw std::runtime_error{"Remote IP must be specified, use the -i or --ip option"};
    }
    if (cli_options.count("port") == 0) {
      throw std::runtime_error{"UDP port must be specified, use the -p or --port option"};
    }

    return options;
  }
  catch (const cxxopts::OptionException& e) {
    throw std::runtime_error{e.what()};
  }
}


int main(int argc, char** argv)
{
  cangw::Options options;
  can::Socket can_socket;
  udp::Socket udp_socket;

  try {
    options = parse_args(argc, argv);
    can_socket.open(options.can_device);
    if (options.listen) {
      can_socket.bind();
      can_socket.set_receive_timeout(3);
    }
    if (options.timestamp)
      can_socket.set_socket_timestamp(true);
    udp_socket.open(options.remote_ip, options.data_port);  // Transmit frames to remote device
    if (options.send) {
      udp_socket.bind("0.0.0.0", options.data_port);  // Receive frames from remote device
      udp_socket.set_receive_timeout(3);
    }
  }
  catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  std::cout << "Routing frames between " << options.can_device << " and " << options.remote_ip
      << ":" << options.data_port << "\nPress enter to stop..." << std::endl;

  std::atomic<bool> stop{false};
  std::thread listener{};
  std::thread sender{};

  if (options.listen) {
    listener = std::thread{&route_to_udp, std::ref(can_socket), std::ref(udp_socket),
        std::ref(stop), options.timestamp};
  }

  if (options.send) {
    sender = std::thread{&route_to_can, std::ref(can_socket), std::ref(udp_socket),
        std::ref(stop)};
  }

  if (options.realtime) {
    // Only attempt to set active threads to realtime scheduling policy
    if ((!listener.joinable() || priority::set_realtime(listener.native_handle())) &&
        (!sender.joinable() || priority::set_realtime(sender.native_handle())))
      std::cout << "Gateway thread(s) set to realtime scheduling policy\n";
    else
      std::cout << "Warning: Could not set scheduling policy, forgot sudo?\n";
  }

  std::cin.ignore();  // Wait in main thread

  std::cout << "Stopping gateway..." << std::endl;
  stop.store(true);
  if (listener.joinable())
    listener.join();
  if (sender.joinable())
    sender.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
