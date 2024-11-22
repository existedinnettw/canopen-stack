#include "sche_util.hpp"

#include <cstdio>
#include <time.h>

/**
 * @brief
 * time1 - time0
 * @see https://stackoverflow.com/a/68804612
 */
static inline struct timespec
diff_timespec(const struct timespec time1, const struct timespec time0)
{
  //   assert(time1);
  //   assert(time0);
  struct timespec diff = { .tv_sec = time1.tv_sec - time0.tv_sec, //
                           .tv_nsec = time1.tv_nsec - time0.tv_nsec };
  if (diff.tv_nsec < 0) {
    diff.tv_nsec += 1000000000; // nsec/sec
    diff.tv_sec--;
  }
  return diff;
}
bool
operator<(const timespec& lhs, const timespec& rhs)
{
  if (lhs.tv_sec == rhs.tv_sec)
    return lhs.tv_nsec < rhs.tv_nsec;
  else
    return lhs.tv_sec < rhs.tv_sec;
}
bool
operator>(const timespec& lhs, const timespec& rhs)
{
  if (lhs.tv_sec == rhs.tv_sec)
    return lhs.tv_nsec > rhs.tv_nsec;
  else
    return lhs.tv_sec > rhs.tv_sec;
}

/**
 * not sure if outlier will lead too conservative problem during schedule.
 */
void
Sche_high_water_mark::wait_unitl_mark(const struct timespec cycle_start_time)
{
  struct timespec cur_time;
  clock_gettime(CLOCK_MONOTONIC, &cur_time);
  struct timespec cur_diff_time = diff_timespec(cur_time, cycle_start_time);
  // printf("[DEBUG] cur mark time: %ld us\n", cur_diff_time.tv_nsec / 1000);

  if (cur_diff_time > time_after_start_time) {
    // update new time diff value
    time_after_start_time = cur_diff_time;
    printf("[DEBUG] new high time mark: %ld us\n", time_after_start_time.tv_nsec / 1000);
  }

  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_after_start_time, NULL);
}

struct timespec
Sche_high_water_mark::get_mark(void)
{
  return time_after_start_time;
}