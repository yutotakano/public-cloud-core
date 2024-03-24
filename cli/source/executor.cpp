#include "executor.h"
#include "exithandler.h"
#include "quill/Quill.h"
#include "subprocess_error.h"
#include "utils.h"
#include <future>
#include <iostream>
#include <string>

Executor::Executor() { logger = quill::get_logger(); }

ExecutingProcess Executor::run(
  std::string command,
  bool stream_cout,
  bool suppress_err,
  bool show_command
)
{
  LOG_DEBUG(logger, "Running command: {}", command);

  // Split the command by spaces, since subprocess_create takes an array
  std::vector<std::string> command_parts = Utils::split(command, ' ');

  return run(command_parts, stream_cout, suppress_err);
}
ExecutingProcess Executor::run(
  std::vector<std::string> command_parts,
  bool stream_cout,
  bool suppress_err,
  bool show_command
)
{
  if (show_command)
  {
    LOG_INFO(logger, "Running command: {}", command_parts);
  }
  else
  {
    LOG_DEBUG(logger, "Running command: {}", command_parts);
  }

  std::string output;
  std::string error;

  // Create the subprocess, inheriting the environment variables from the
  // current process
  std::shared_ptr<TinyProcessLib::Process> process =
    std::make_shared<TinyProcessLib::Process>(
      command_parts,
      "",
      [&, stream_cout, suppress_err](const char *bytes, size_t n)
      {
        // Callback for stdout
        if (n > 0)
        {
          LOG_TRACE_L3(logger, "{} bytes read from stdout", n);
        }

        output += std::string(bytes, n);
        if (stream_cout)
        {
          std::cout << std::string(bytes, n);
        }
      },
      [&, stream_cout, suppress_err](const char *bytes, size_t n)
      {
        // Callback for stderr
        if (n > 0)
        {
          LOG_TRACE_L3(logger, "{} bytes read from stderr", n);
        }

        error += std::string(bytes, n);
        if (!suppress_err)
        {
          std::cerr << std::string(bytes, n);
        }
      }
    );

  // Create the ExecutingProcess struct that will be returned to the caller
  auto result = ExecutingProcess();
  result.subprocess = process;
  result.future = std::async(
    [this, process, &output, stream_cout, suppress_err]()
    {
      int exit_status = 0;
      while (!process->try_get_exit_status(exit_status))
      {
        // Check if program has received Ctrl-C, since we are in another thread
        // and won't receive it. Wait up to 100ms to prevent busy-waiting on the
        // subprocess. Note that the runtime error here will stop this thread,
        // but not the main thread directly, unless .get() is used on the
        // future.
        if (ExitHandler::exit_condition_met(100))
        {
          LOG_INFO(logger, "Aborting subprocess due to exit condition");
          process->kill();
          throw SubprocessUserAbort("Aborting due to user interrupt");
        }

        // Sleep for a bit to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
      }

      LOG_DEBUG(logger, "Process exited with code: {}", exit_status);

      if (exit_status != 0)
      {
        // Throw an exception and abort if the process exited badly
        if (!suppress_err)
          throw SubprocessError("Subprocess exited with non-zero code");
      }

      return Utils::trim(output);
    }
  );

  return result;
}

void Executor::print_versions()
{
  LOG_INFO(logger, "Printing versions...");

  auto docker_version = run("docker --version", false).future.get();
  LOG_INFO(logger, "Docker version: {}", docker_version);

  auto kubectl_version = run("kubectl version --client", false).future.get();
  LOG_INFO(logger, "Kubectl version: {}", kubectl_version);

  auto aws_version = run("aws --version", false).future.get();
  LOG_INFO(logger, "AWS version: {}", aws_version);

  auto eksctl_version = run("eksctl version", false).future.get();
  LOG_INFO(logger, "Eksctl version: {}", eksctl_version);
}
