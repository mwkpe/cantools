/* A command line program for transmitting CAN frames over UDP
 */


#include <linux/can.h>
#include <unistd.h>

#include <cstdint>
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>

#include "cxxopts.hpp"

#include "timer.h"
#include "udpsocket.h"


// Evil functions to defeat the optimizer
__attribute__((unused)) static void escape(void* p) { asm volatile("" : : "g"(p) : "memory"); }
__attribute__((unused)) static void clobber() { asm volatile("" : : : "memory"); }


void simulate(std::atomic<bool>& stop, std::string&& ip, std::uint16_t port)
{
  // Example frames
  can_frame vehicle{0};
  vehicle.can_id = 0xC9;
  vehicle.can_dlc = 4;
  vehicle.data[0] = 0xBB;
  vehicle.data[1] = 0xFF;

  can_frame radar{0};
  radar.can_id = 0xCA;
  radar.can_dlc = 4;
  // Motorola byte order
  // Pos 7, len 14 = 12317
  // Pos 9, len 14 = 4404
  // Pos 27, len 4 = 11
  radar.data[0] = 0b1100'0000;
  radar.data[1] = 0b0111'0101;
  radar.data[2] = 0b0001'0011;
  radar.data[3] = 0b0100'1011;

  // Network inferface and timer
  udp::socket udp_socket;
  util::timer timer;
  try {
    udp_socket.open(ip, port);
    timer.init_system_timer();
  }
  catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return;
  }

  // Transmit scheduling
  auto transmit = [&](std::uint64_t time_ms) {
    if (time_ms % 500 == 0) {  // 500 ms cycle time
      vehicle.data[0]++;
      udp_socket.transmit(&vehicle);
    }
    if (time_ms % 200 == 0) {  // 200 ms cycle time
      udp_socket.transmit(&radar);
    }
  };

  // Looping and timing
  auto current_time_us = timer.system_time();
  escape(&current_time_us);  // Prevent infinite loop due to optimizer fixing this value
  auto next_cycle = current_time_us + 1000ull;

  while (!stop.load())
  {
    do {
      current_time_us = timer.system_time();
    } while (current_time_us < next_cycle);  // Polling wait until exactly 1 ms passed
    next_cycle = current_time_us + 1000ull;
    transmit(current_time_us / 1000ull);
    usleep(750);
  }
}


std::tuple<std::string, std::uint16_t> parse_args(int argc, char** argv)
{
  std::string remote_ip;
  std::uint16_t remote_port;

  try {
    cxxopts::Options options{"cangw", "CAN to UDP router"};
    options.add_options()
      ("ip", "Remote device IP", cxxopts::value<std::string>(remote_ip))
      ("port", "UDP port", cxxopts::value<std::uint16_t>(remote_port))
    ;
    options.parse(argc, argv);

    if (options.count("ip") == 0) {
      throw std::runtime_error{"Remote IP must be specified, use the --ip option"};
    }
    if (options.count("port") == 0) {
      throw std::runtime_error{"UDP port must be specified, use the --port option"};
    }

    return std::make_tuple(std::move(remote_ip), remote_port);
  }
  catch (const cxxopts::OptionException& e) {
    throw std::runtime_error{e.what()};
  }
}


int main(int argc, char** argv)
{
  std::string remote_ip;
  std::uint16_t remote_port;

  try {
    std::tie(remote_ip, remote_port) = parse_args(argc, argv);
  }
  catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  std::cout << "Sending frames to " << remote_ip << ":" << remote_port
      << "\nPress enter to stop..." << std::endl;

  std::atomic<bool> stop{false};
  std::thread simulation{&simulate, std::ref(stop), std::move(remote_ip), remote_port};
  std::cin.ignore();  // Wait in main thread

  std::cout << "Stopping simulation..." << std::endl;
  stop.store(true);
  simulation.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
