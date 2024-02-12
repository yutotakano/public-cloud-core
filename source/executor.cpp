#include "executor.h"
#include "exithandler.h"
#include "quill/Quill.h"
#include "subprocess_error.h"
#include "utils.h"
#include <future>
#include <iostream>
#include <string>

Executor::Executor() { logger = quill::get_logger(); }

ExecutingProcess Executor::run(std::string command, bool stream_cout)
{
  LOG_DEBUG(logger, "Running command: {}", command);

  // Split the command by spaces, since subprocess_create takes an array
  std::vector<std::string> command_parts = Utils::split(command, ' ');

  return run(command_parts, stream_cout);
}
ExecutingProcess
Executor::run(std::vector<std::string> command_parts, bool stream_cout)
{
  // If streaming to stdout, we want to make sure the command is visible, but
  // otherwise the user doesn't need to see it unless they are debugging.
  if (stream_cout)
  {
    LOG_INFO(logger, "Running command: {}", command_parts);
  }
  else
  {
    LOG_DEBUG(logger, "Running command: {}", command_parts);
  }

  // Convert the command parts to a C-style array of C-style strings by
  // iterating over the vector and pushing the c_str() of each string into a
  // reserved vector of char pointers.
  std::vector<char *> cstr_command_parts;
  cstr_command_parts.reserve(command_parts.size());

  for (size_t i = 0; i < command_parts.size(); ++i)
  {
    cstr_command_parts.push_back(const_cast<char *>(command_parts[i].c_str()));
  }
  // Push NULL at the end since subprocess_create requires it to know when the
  // array ends.
  cstr_command_parts.push_back(nullptr);

  // Create the subprocess struct that will contain the process handles - make
  // it shared as it will be used in the future (in the async lambda below) and
  // we want to keep it alive until the future is done.
  std::shared_ptr<subprocess_s> process = std::make_shared<subprocess_s>();
  int process_result = subprocess_create(
    &cstr_command_parts[0],
    // enable_async is required to use the subprocess_read_stdout and
    // subprocess_read_stderr functions, which are used to stream the output of
    // the process instead of waiting for the process to end.
    subprocess_option_enable_async | subprocess_option_inherit_environment,
    process.get()
  );

  // Sanity check the result of subprocess_create
  if (process_result != 0)
  {
    LOG_ERROR(logger, "Error creating subprocess: {}", process_result);
  }

  // Create the ExecutingProcess struct that will be returned to the caller
  auto result = ExecutingProcess();
  result.subprocess = process.get();
  result.future = std::async(
    [this, stream_cout, process]()
    {
      // Stream the output of the process stdout to a small buffer that gets
      // processed into the output string
      char out_buffer[1024] = {0};
      char err_buffer[1024] = {0};
      std::string output;
      std::string error;
      int bread = 0;

      while (subprocess_alive(process.get()))
      {
        // Check if program has received Ctrl-C, since we are in another thread
        // and won't receive it. Wait up to 100ms to prevent busy-waiting on the
        // subprocess. Note that the runtime error here will stop this thread,
        // but not the main thread directly, unless .get() is used on the
        // future.
        if (ExitHandler::exit_condition_met(100))
        {
          LOG_INFO(logger, "Aborting subprocess due to exit condition");
          throw SubprocessUserAbort("Aborting due to user interrupt");
        }

        // Zero out the buffers before reading into them
        std::fill(std::begin(out_buffer), std::end(out_buffer), 0);
        std::fill(std::begin(err_buffer), std::end(err_buffer), 0);

        bread = subprocess_read_stdout(process.get(), out_buffer, 1024);
        if (bread > 0)
        {
          LOG_TRACE_L3(logger, "{} bytes read from stdout", bread);
        }

        output += out_buffer;
        if (stream_cout)
          std::cout << out_buffer;

        bread = subprocess_read_stderr(process.get(), err_buffer, 1024);
        if (bread > 0)
        {
          LOG_TRACE_L3(logger, "{} bytes read from stderr", bread);
        }

        error += err_buffer;
      }

      // Get exit code and log it
      int exit_code;
      subprocess_join(process.get(), &exit_code);
      LOG_DEBUG(logger, "Process exited with code: {}", exit_code);

      if (exit_code != 0)
      {
        // Throw an exception and abort if the process exited badly
        LOG_ERROR(logger, "Error: {}", error);
        throw SubprocessError("Subprocess exited with non-zero code");
      }
      else if (!error.empty())
      {
        // Log the errors here, since the future will return only the stdout for
        // convenience.
        LOG_WARNING(logger, "Warning: {}", error);
      }

      return output;
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
