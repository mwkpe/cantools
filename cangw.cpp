/* A small command line program for routing frames between a CAN socket and an UDP socket
 */


#include <sched.h>
#include <pthread.h>

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
  std::vector<std::uint8_t> buffer(sizeof(std::uint64_t) + sizeof(can_frame));
  // Passing on original receive time for more accurate cycle time information of frames
  auto* time = reinterpret_cast<std::uint64_t*>(buffer.data());
  auto* frame = reinterpret_cast<can_frame*>(buffer.data() + sizeof(std::uint64_t));

  while (!stop.load()) {
    // Anciallary data is not part of socket payload
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


void set_scheduling_priority(std::thread& thread)
{
  sched_param sch;
  int policy;
  int priority = sched_get_priority_max(SCHED_FIFO);
  sch.sched_priority = priority != -1 ? priority : 1;
  if (pthread_setschedparam(thread.native_handle(), SCHED_FIFO, &sch) == 0) {
    if (pthread_getschedparam(thread.native_handle(), &policy, &sch) == 0) {
      std::cout << "Scheduling policy set to ";
      switch (policy) {
        case SCHED_OTHER: std::cout << "SCHED_OTHER"; break;
        case SCHED_FIFO: std::cout << "SCHED_FIFO"; break;
        case SCHED_RR: std::cout << "SCHED_RR"; break;
      }
      std::cout << " with priority " << sch.sched_priority << std::endl;
    }
  }
  else {
    throw std::runtime_error{"Could not set thread scheduling policy and priority, forgot sudo?"};
  }
}


std::tuple<std::string, std::string, std::uint16_t> parse_args(int argc, char** argv)
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


int main(int argc, char** argv)
{
  std::string can_device;
  std::string remote_ip;
  std::uint16_t remote_port;
  can::Socket can_socket;
  udp::Socket udp_socket;

  try {
    std::tie(can_device, remote_ip, remote_port) = parse_args(argc, argv);
    can_socket.open(can_device);
    can_socket.bind();
    can_socket.set_receive_timeout(3);
    can_socket.set_socket_timestamp(true);
    udp_socket.open(remote_ip, remote_port);
    udp_socket.bind("0.0.0.0", remote_port);
    udp_socket.set_receive_timeout(3);
  }
  catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  std::cout << "Routing frames between " << can_device << " and " << remote_ip << ":"
      << remote_port << "\nPress enter to stop..." << std::endl;

  std::atomic<bool> stop{false};
  std::thread can_to_udp{&route_to_udp_with_timestamp, std::ref(can_socket),
      std::ref(udp_socket), std::ref(stop)};
  std::thread udp_to_can{&route_to_can, std::ref(can_socket), std::ref(udp_socket), std::ref(stop)};
  try {
    set_scheduling_priority(can_to_udp);
    set_scheduling_priority(udp_to_can);
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Warning: " << e.what() << std::endl;
  }
  std::cin.ignore();  // Wait in main thread

  std::cout << "Stopping gateway..." << std::endl;
  stop.store(true);
  can_to_udp.join();
  udp_to_can.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
