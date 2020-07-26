#ifndef JAK_V2_TIMER_H
#define JAK_V2_TIMER_H

#include <cassert>
#include <cstdint>
#include <ctime>

/*!
 * Timer for measuring time elapsed with clock_monotonic
 */
class Timer {
public:

  /*!
   * Construct and start timer
   */
  explicit Timer() { start(); }

  /*!
   * Start the timer
   */
  void start() { clock_gettime(CLOCK_MONOTONIC, &_startTime); }

  /*!
   * Get milliseconds elapsed
   */
  double getMs() const { return (double)getNs() / 1.e6; }

  double getUs() const {
    return (double)getNs() / 1.e3;
  }

  /*!
   * Get nanoseconds elapsed
   */
  int64_t getNs() const {
    struct timespec now = {};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t)(now.tv_nsec - _startTime.tv_nsec) +
           1000000000 * (now.tv_sec - _startTime.tv_sec);
  }

  /*!
   * Get seconds elapsed
   */
  double getSeconds() const { return (double)getNs() / 1.e9; }

  struct timespec _startTime = {};
};


#endif //JAK_V2_TIMER_H
