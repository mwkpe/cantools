/* A small command line program for a one-time or cyclic transmission of a single frame
 */


#include <string>
#include <tuple>
#include <chrono>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <ios>

#include "cxxopts.hpp"
#include "cansocket.h"


can_frame build_frame(const std::string& id, const std::string& data)
{
  // Invalid inputs will result in id and data set to 0
  can_frame frame;
  frame.can_id = std::strtoul(id.c_str(), nullptr, 16) & 0x7FF;  // Ignoring extended format
  frame.can_dlc = (data.size() + 1) / 2;
  if (frame.can_dlc > CAN_MAX_DLC)
    frame.can_dlc = CAN_MAX_DLC;
  auto d = std::strtoull(data.c_str(), nullptr, 16);
  // Reversing order of bytes so a 010203 string is 010203 in the data field
  int n = 0;
  int i = frame.can_dlc;
  while (i--)
    frame.data[i] = d >> n++ * 8;  // Place lowest byte using highest index
  return frame;
}


void print_frame(const can_frame& frame)
{
  std::cout << "id: " << std::hex << frame.can_id << std::dec << ", dlc: "
      << static_cast<int>(frame.can_dlc) << ", data: " << std::hex << std::setfill('0');
  for (int i=0; i<frame.can_dlc; i++) {
    std::cout << std::setw(2) << static_cast<int>(frame.data[i]) << ' ';
  }
  std::cout << '\n';
  std::cout.copyfmt(std::ios{nullptr});  // Reset format state
}


void transmit_frame(std::atomic<bool>& transmit_cyclical, const std::string device, can_frame frame,
    int cycle_time)
{
  can::Socket can_socket;
  try {
    can_socket.open(device);
  }
  catch (const can::Socket_error& e) {
    std::cerr << e.what() << std::endl;
    return;
  }

  do
  {
    if (can_socket.transmit(&frame) != sizeof(frame)) {
      std::cerr << "Socket write error" << std::endl;
      return;
    }
    if (cycle_time > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(cycle_time));
    }
  } while (transmit_cyclical.load());
}


std::tuple<std::string, can_frame, int> parse_args(int argc, char** argv)
{
  std::string device;
  std::string id;
  std::string data;
  int cycle_time;

  try {
    cxxopts::Options options{"cantx", "CAN message transmitter"};
    options.add_options()
      ("id", "Hex message id", cxxopts::value<std::string>(id))
      ("data", "Hex data string", cxxopts::value<std::string>(data)->default_value("00"))
      ("cycle", "Cycle time in ms", cxxopts::value<int>(cycle_time)->default_value("-1"))
      ("device", "CAN device name", cxxopts::value<std::string>(device)->default_value("can0"))
    ;
    options.parse(argc, argv);

    if (options.count("id") == 0) {
      throw std::runtime_error{"Message ID must be specified, use --id option"};
    }
    if (data.size() % 2 || data.size() > 16) {
      throw std::runtime_error{"Data size error, size must be even and <= 16"};
    }

    return std::make_tuple(std::move(device), build_frame(id, data), cycle_time);
  }
  catch (const cxxopts::OptionException& e) {
    throw std::runtime_error{e.what()};
  }
}


int main(int argc, char** argv)
{
  std::string device;
  can_frame frame;
  int cycle_time;

  try {
    std::tie(device, frame, cycle_time) = parse_args(argc, argv);
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Error parsing command line options:\n" << e.what() << std::endl;
    return 1;
  }

  std::atomic<bool> transmit_cyclical{cycle_time > 0};

  std::cout << "Transmitting frame on " << device;
  if (transmit_cyclical.load())
    std::cout << " each " << cycle_time << " ms...";
  std::cout << '\n';
  print_frame(frame);
  if (transmit_cyclical.load())
    std::cout << "Press enter to stop..." << std::endl;

  std::thread transmitter{&transmit_frame, std::ref(transmit_cyclical), device, frame, cycle_time};

  if (transmit_cyclical.load())
    std::cin.ignore();  // Wait in main thread

  std::cout << "Stopping transmitter..." << std::endl;
  transmit_cyclical.store(false);
  transmitter.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
