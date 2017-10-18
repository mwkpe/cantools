/* A small command line program for printing frames into the console
 */


#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <ios>

#include "cxxopts.hpp"
#include "cansocket.h"


void print_frame(const can_frame& frame, std::uint64_t time)
{
  std::cout << time << std::setfill(' ') << std::hex << std::setw(8) << frame.can_id << std::dec
      << "   " << static_cast<int>(frame.can_dlc) << "   " << std::hex << std::setfill('0');
  for (int i=0; i<frame.can_dlc; i++) {
    std::cout << std::setw(2) << static_cast<int>(frame.data[i]) << ' ';
  }
  std::cout << '\n';
  std::cout.copyfmt(std::ios{nullptr});  // Reset format state
}


void print_frames(std::atomic<bool>& stop, std::string device)
{
  can::Socket can_socket;
  try {
    can_socket.open(device);
    can_socket.bind();
    can_socket.set_receive_timeout(3);
    can_socket.set_socket_timestamp(true);
  }
  catch (const can::Socket_error& e) {
    std::cerr << e.what() << std::endl;
    return;
  }

  std::uint64_t time;
  can_frame frame;

  while (!stop.load()) {
    auto n = can_socket.receive(&frame, &time);
    if (n == CAN_MTU) {
      print_frame(frame, time);
    }
    else if (n > 0) {
      std::cout << "Received incomplete frame (" << n << " bytes)" << std::endl;
    }
    else if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        std::cout << "Receive timeout" << std::endl;
      else
        std::cout << "Unknown error" << std::endl;
    }
  }
}


std::string parse_args(int argc, char** argv)
{
  std::string can_device;

  try {
    cxxopts::Options options{"canprint", "Prints CAN frames to console"};
    options.add_options()
      ("device", "CAN device name", cxxopts::value<std::string>(can_device)->default_value("can0"))
    ;
    options.parse(argc, argv);
    return can_device;
  }
  catch (const cxxopts::OptionException& e) {
    throw std::runtime_error{e.what()};
  }
}


int main(int argc, char** argv)
{
  std::string can_device;

  try {
    can_device = parse_args(argc, argv);
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Error parsing command line options:\n" << e.what() << std::endl;
    return 1;
  }

  std::cout << "Printing frames from " << can_device << "\nPress enter to stop..." << std::endl;

  std::atomic<bool> stop{false};
  std::thread printer{&print_frames, std::ref(stop), can_device};
  std::cin.ignore();  // Wait in main thread

  std::cout << "Stopping printer..." << std::endl;
  stop.store(true);
  printer.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
