#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "process.hpp"
#include "quill/Quill.h"
#include <future>
#include <string>

struct ExecutingProcess
{
  std::future<std::string> future;
  std::shared_ptr<TinyProcessLib::Process> subprocess;
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
   * @param show_command Whether to log the command being run
   * @return ExecutingProcess
   */
  ExecutingProcess run(
    std::string command,
    bool stream_cout = true,
    bool suppress_err = false,
    bool show_command = true
  );

  /**
   * @brief Run a command, optionally passing a pipe or string to stdin. Will
   * return a struct containing the future for the command's stdout, as well as
   * the raw process handles in case the caller needs to interact with the
   * process directly.
   *
   * @param command Command to run, e.g. "ls -l"
   * @param stream_cout Whether to stream the stdout of the command
   * @param suppress_err Whether to suppress logging stderr. Will still raise
   * exceptions if the command fails.
   * @param show_command Whether to log the command being run
   * @return ExecutingProcess
   */
  ExecutingProcess run(
    std::vector<std::string> command_parts,
    bool stream_cout = true,
    bool suppress_err = false,
    bool show_command = true
  );

  void show_help_message(
    std::vector<std::string> command_parts,
    std::string &error,
    std::string &output
  );

  /**
   * @brief Print the versions of the tools used by the executor.
   */
  void print_versions();

private:
  quill::Logger *logger;
};

#endif // EXECUTOR_H
