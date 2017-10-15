/* A small timer class for the raspberry pi system timer
 */


#ifndef UTIL_TIMER_H
#define UTIL_TIMER_H


#include <cstdint>
#include <stdexcept>


#define ST_BASE_RPI_1_AND_ZERO 0x20003000
#define ST_BASE_RPI_2_AND_3 0x3F003000
#define TIMER_OFFSET 4


namespace util
{


class Timer_error : public std::runtime_error
{
public:
  Timer_error(const std::string& s) : std::runtime_error{s} {}
  Timer_error(const char* s) : std::runtime_error{s} {}
};


class Timer
{
public:
  Timer() : st_time{&st_dummy} {}
  void init_system_timer();  // 64-bit microsecond system timer (requires sudo)
  std::uint64_t epoch_time() const;
  std::uint64_t system_time() const { return *st_time; }

private:
  // 64-bit system timer
  std::uint64_t st_dummy{0};
  std::uint64_t* st_time{nullptr};
};


}  // namespace util


#endif  // UTIL_TIMER_H
