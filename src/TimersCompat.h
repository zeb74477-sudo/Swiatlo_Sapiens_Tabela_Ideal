#ifndef TIMERS_COMPAT_H
#define TIMERS_COMPAT_H

// Compatibility wrapper for the older `Timers` library used in this project.
// Some sketches expect a `Timer` type and helper macros like `SECS()`.
// This header adapts the `Timers` API to that expectation.

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

// Alias for clarity
using TimerAlias = Timer;

#define SECS(x) ((uint32_t)((x) * 1000UL))

#endif
