#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "quill/Quill.h"
#include "subprocess/subprocess.h"
#include <future>
#include <string>

struct ExecutingProcess
{
  std::future<std::string> future;
  std::string command;
  subprocess_s *subprocess;
};

class Executor
{
public:
  Executor();

  /**
   * @brief Run a command, optionally passing a pipe or string to stdin. Will
   * return a struct containing the future for the command's stdout, as well as
   * the raw process handles in case the caller needs to interact with the
   * process directly.
   *
   * @param command Command to run, e.g. "ls -l"
   * @return ExecutingProcess
   */
  ExecutingProcess run(std::string command);

  /**
   * @brief Print the versions of the tools used by the executor.
   */
  void print_versions();

private:
  quill::Logger *logger;
};

#endif // EXECUTOR_H
