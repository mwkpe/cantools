#include "timer.h"


#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <chrono>


void util::Timer::init_system_timer()
{
  auto fd = ::open("/dev/mem", O_RDONLY);
  if (fd == -1)
    throw Timer_error{"System timer init error: Can't read /dev/mem, forgot sudo?"};

  auto* m = ::mmap(nullptr, 4096, PROT_READ, MAP_SHARED, fd, ST_BASE_RPI_2_AND_3);
  if (m == MAP_FAILED)
    throw Timer_error{"System timer init error: Can't map memory"};

  st_time = reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uint8_t*>(m) + TIMER_OFFSET);
}


std::uint64_t util::Timer::epoch_time() const
{
  return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}
