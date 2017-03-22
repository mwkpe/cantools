/* A small command line program for the single or cyclic transmission
   of a CAN message adapted from cansend of the linux can utils
 */


#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <string>
#include <cstring>
#include <tuple>
#include <chrono>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <ios>

#include "cxxopts.hpp"
#include "open_socket.h"


can_frame parse_frame(const std::string& id, const std::string& data)
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
  std::cout.copyfmt(std::ios(nullptr));  // Reset format state
}


void send_frame(std::atomic<bool>& stop_transmission, const std::string device,
    can_frame frame, int cycle_time)
{
  sockaddr_can addr;
  ifreq ifr;
  open_socket s{socket(PF_CAN, SOCK_RAW, CAN_RAW)};

  if (s.id < 0) {
    std::cerr << "Error while creating socket, aborted" << std::endl;
    return;
  }

  addr.can_family = AF_CAN;

  std::strcpy(ifr.ifr_name, device.c_str());
  if (ioctl(s.id, SIOCGIFINDEX, &ifr) < 0) {
    std::cerr << "Error retrieving interface index of device, aborted" << std::endl;
    return;
  }
  addr.can_ifindex = ifr.ifr_ifindex;

  setsockopt(s.id, SOL_CAN_RAW, CAN_RAW_FILTER, nullptr, 0);

  if (bind(s.id, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "Error while binding socket, aborted" << std::endl;
    return;
  }

  if (cycle_time < 0)
    stop_transmission.store(true);

  do
  {
    if (write(s.id, &frame, sizeof(frame)) != sizeof(frame)) {
      std::cerr << "Socket write error, aborted" << std::endl;
      return;
    }
    if (cycle_time > 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(cycle_time));
  } while (!stop_transmission.load());
}


std::tuple<std::string, can_frame, int> parse_args(int argc, char **argv)
{
  std::string device;
  int cycle_time;

  try {
    std::string id;
    std::string data;
    cxxopts::Options options("cantx", "CAN message transmitter");
    options.add_options()
      ("id", "Hex message id", cxxopts::value<std::string>(id))
      ("data", "Hex data string", cxxopts::value<std::string>(data)->default_value("00"))
      ("cycle", "Cycle time in ms", cxxopts::value<int>(cycle_time)->default_value("-1"))
      ("device", "CAN device name", cxxopts::value<std::string>(device)->default_value("can0"))
    ;
    options.parse(argc, argv);
    if (options.count("id") == 0) {
      throw std::runtime_error("Message ID must be specified, use --id option");
    }
    if (data.size() % 2 || data.size() > 16) {
      throw std::runtime_error("Data size error, size must be even and <= 16");
    }

    return std::make_tuple(std::move(device), parse_frame(id, data), cycle_time);
  }
  catch (const cxxopts::OptionException& e) {
    throw std::runtime_error(e.what());
  }
}


int main(int argc, char **argv)
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

  std::cout << "Transmitting frame on " << device;
  if (cycle_time > 0)
    std::cout << " each " << cycle_time << " ms...";
  std::cout << '\n';
  print_frame(frame);
  if (cycle_time > 0)
    std::cout << "Press enter to stop" << std::endl;

  std::atomic<bool> stop_transmission{false};
  std::thread transmitter{&send_frame, std::ref(stop_transmission), device, frame, cycle_time};

  if (cycle_time > 0)
    std::cin.ignore();

  stop_transmission.store(true);
  transmitter.join();

  return 0;
}
