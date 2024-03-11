#include "exithandler.h"
#include "ctrl-c/ctrl-c.h"
#include <iostream>

void ExitHandler::create_handlers()
{
  // As we want to register the handler only once (like a singleton), we set the
  // static initialized field once it's been done once
  if (initialized)
    return;

  initialized = true;

  unsigned int handler_id = CtrlCLibrary::SetCtrlCHandler(
    [](enum CtrlCLibrary::CtrlSignal event) -> bool
    {
      switch (event)
      {
      case CtrlCLibrary::kCtrlCSignal:
        // Get a lock for modifying the exit_condition, then wake all .wait()s
        // on that condition
        std::lock_guard<std::mutex> locker(exit_condition_mutex);
        std::cout << "Caught Ctrl+C signal" << std::endl;
        exiting = true;

        // Allow threads to clean up (hopefully). If they raise any exceptions,
        // they won't be seen since the main thread is currently here, and will
        // exit with an exception in the next line before any calls of .get()
        // have a chance to run and re-throw child thread exceptions.
        exit_condition.notify_all();

        throw std::runtime_error("Aborting main thread due to user interrupt");
      }
      return true;
    }
  );

  if (handler_id == CtrlCLibrary::kErrorID)
  {
    std::cerr << "Could not set interrupt handler - program may not respond to "
                 "Ctrl+C or SIGTERM"
              << std::endl;
  }
}

bool ExitHandler::exit_condition_met(int timeout)
{
  std::unique_lock<std::mutex> locker(exit_condition_mutex);
  auto status =
    exit_condition.wait_for(locker, std::chrono::milliseconds(timeout));
  return status == std::cv_status::no_timeout || exiting == true;
}
