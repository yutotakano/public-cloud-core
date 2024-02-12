#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "quill/Quill.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion-null"
#include "subprocess/subprocess.h"
#pragma GCC diagnostic pop
#include <future>
#include <string>

struct ExecutingProcess
{
  std::future<std::string> future;
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
   * @param stream_cout Whether to stream the stdout of the command
   * @return ExecutingProcess
   */
  ExecutingProcess run(std::string command, bool stream_cout = true);

  /**
   * @brief Run a command, optionally passing a pipe or string to stdin. Will
   * return a struct containing the future for the command's stdout, as well as
   * the raw process handles in case the caller needs to interact with the
   * process directly.
   *
   * @param command Command to run, e.g. "ls -l"
   * @param stream_cout Whether to stream the stdout of the command
   * @return ExecutingProcess
   */
  ExecutingProcess
  run(std::vector<std::string> command_parts, bool stream_cout = true);

  /**
   * @brief Print the versions of the tools used by the executor.
   */
  void print_versions();

private:
  quill::Logger *logger;
};

#endif // EXECUTOR_H
