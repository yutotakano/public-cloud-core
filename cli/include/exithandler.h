#ifndef EXITHANDLER_H
#define EXITHANDLER_H

#include <condition_variable>

class ExitHandler
{
public:
  /**
   * @brief Initializes the singleton by registering the interrupt handlers.
   */
  static void create_handlers();

  /**
   * @brief Check if an exit has been triggered by a Ctrl-C interrupt,
   * optionally waiting for 'timeout' milliseconds.
   *
   * @param timeout Milliseconds to wait for a response
   * @return true If exit condition is met
   * @return false
   */
  static bool exit_condition_met(int timeout = 0);

private:
  static inline bool initialized = false;

  static inline std::mutex exit_condition_mutex;
  static inline std::condition_variable exit_condition;

  // The condition variable only acts as an event notifier, so we need some
  // persistent method of knowing whether the program is exiting.
  static inline bool exiting = false;
};
#endif
