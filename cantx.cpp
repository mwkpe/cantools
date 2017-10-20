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
#include "priority.h"


can_frame build_frame(const std::string& id, const std::string& data)
{
  // Invalid inputs will result in id and data set to 0
  can_frame frame;
  frame.can_id = std::strtoul(id.c_str(), nullptr, 16) & 0x7FF;  // Ignoring extended format
  frame.can_dlc = (data.size() + 1) / 2;
  if (frame.can_dlc > CAN_MAX_DLC)
    frame.can_dlc = CAN_MAX_DLC;

  auto d = std::strtoull(data.c_str(), nullptr, 16);
  for (int i=0; i<frame.can_dlc; ++i)
    frame.data[i] = d >> i * 8;

  return frame;
}


void print_frame(const can_frame& frame)
{
  std::cout << "id: " << std::hex << frame.can_id << std::dec << ", dlc: "
      << static_cast<int>(frame.can_dlc) << ", data: " << std::hex << std::setfill('0');
  for (int i=frame.can_dlc-1; i>=0; --i) {
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


std::tuple<std::string, can_frame, int, bool> parse_args(int argc, char** argv)
{
  std::string device;
  std::string id;
  std::string payload;
  int cycle_time;
  bool realtime = false;

  try {
    cxxopts::Options options{"cantx", "CAN message transmitter"};
    options.add_options()
      ("i,id", "Hex frame ID", cxxopts::value<std::string>(id))
      ("p,payload", "Hex data string", cxxopts::value<std::string>(payload)->default_value("00"))
      ("c,cycle", "Cycle time in ms", cxxopts::value<int>(cycle_time)->default_value("-1"))
      ("d,device", "CAN device name", cxxopts::value<std::string>(device)->default_value("can0"))
      ("r,realtime", "Enable realtime scheduling policy", cxxopts::value<bool>(realtime))
    ;
    options.parse(argc, argv);

    if (options.count("id") == 0) {
      throw std::runtime_error{"Message ID must be specified, use -i or --id option"};
    }
    if (payload.size() % 2 || payload.size() > 16) {
      throw std::runtime_error{"Payload size error, size must be even and <= 16"};
    }

    return std::make_tuple(std::move(device), build_frame(id, payload), cycle_time, realtime);
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
  bool realtime;

  try {
    std::tie(device, frame, cycle_time, realtime) = parse_args(argc, argv);
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

  std::thread transmitter{&transmit_frame, std::ref(transmit_cyclical), device, frame, cycle_time};

  if (transmit_cyclical.load()) {
    if (realtime) {
      if (priority::set_realtime(transmitter.native_handle()))
        std::cout << "Transmitter thread set to realtime scheduling policy\n";
      else
        std::cout << "Warning: Could not set scheduling policy, forgot sudo?\n";
    }
    std::cout << "Press enter to stop..." << std::endl;
    std::cin.ignore();  // Wait in main thread
    std::cout << "Stopping transmitter..." << std::endl;
    transmit_cyclical.store(false);
  }

  transmitter.join();

  std::cout << "Program finished" << std::endl;
  return 0;
}
