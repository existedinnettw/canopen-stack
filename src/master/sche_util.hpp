#pragma once

#include <cstdint>
#include <time.h>

class Sche_high_water_mark
{
public:
  Sche_high_water_mark() {};

  void wait_unitl_mark(const struct timespec cycle_start_time);

  struct timespec get_mark(void);

private:
  struct timespec time_after_start_time
  {
    0, 0
  };
};