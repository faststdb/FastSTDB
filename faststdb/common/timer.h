/*!
 * \file timer.h
 */
#ifndef FASTSTDB_COMMON_TIMER_H_
#define FASTSTDB_COMMON_TIMER_H_

#include <sys/time.h>

namespace faststdb {
namespace common {

class Timer {
 public:
  Timer() { gettimeofday(&start_time_, nullptr); }
  void restart() { gettimeofday(&start_time_, nullptr); }
  double elapsed() const {
    timeval curr;
    gettimeofday(&curr, nullptr);
    return double(curr.tv_sec - start_time_.tv_sec) +
        double(curr.tv_usec - start_time_.tv_usec) / 1000000.0;
  }

 private:
  timeval start_time_;
};

}  // namespace common
}  // namespace faststdb

#endif  // FASTSTDB_COMMON_TIMER_H_
