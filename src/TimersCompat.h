#ifndef TIMERS_COMPAT_H
#define TIMERS_COMPAT_H

#include <Timers.h>

class Timer {
public:
  Timer() {}
  void begin(uint32_t ms) { t.start(ms); }
  void start(uint32_t ms) { t.start(ms); }
  void restart() { t.restart(); }
  void stop() { t.stop(); }
  bool available() { return t.available(); }
private:
  Timers t;
};

#define SECS(x) ((uint32_t)((x) * 1000UL))

#endif
