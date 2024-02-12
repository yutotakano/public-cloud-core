#include "executor.h"
#include "quill/Quill.h"
#include "subprocess/subprocess.h"
#include "utils.h"
#include <future>
#include <iostream>
#include <string>

Executor::Executor() { logger = quill::get_logger(); }

ExecutingProcess Executor::run(std::string command)
{
  LOG_DEBUG(logger, "Running command: {}", command);

  // Split the command by spaces, since subprocess_create takes an array
  std::vector<std::string> command_parts = Utils::split(command, ' ');
  LOG_DEBUG(logger, "Raw command: \"{}\"", command_parts);

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
  result.command = command;
  result.subprocess = process.get();
  result.future = std::async(
    [this, process]()
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
        // Sleep for a bit to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        bread = subprocess_read_stdout(process.get(), out_buffer, 1024);
        if (bread > 0)
        {
          LOG_TRACE_L3(logger, "{} bytes read from stdout", bread);
          LOG_TRACE_L3(logger, "Output: {}", out_buffer);
        }

        output += out_buffer;

        bread = subprocess_read_stderr(process.get(), err_buffer, 1024);
        if (bread > 0)
        {
          LOG_TRACE_L3(logger, "{} bytes read from stderr", bread);
          LOG_TRACE_L3(logger, "Output: {}", err_buffer);
        }

        error += err_buffer;
      }

      // Log the error here, since the future will return only the stdout for
      // convenience.
      if (!error.empty())
      {
        LOG_ERROR(logger, "Error: {}", error);
      }

      return output;
    }
  );

  return result;
}

void Executor::print_versions()
{
  LOG_INFO(logger, "Printing versions...");

  auto docker_version = run("docker --version").future.get();
  LOG_INFO(logger, "Docker version: {}", docker_version);

  auto kubectl_version = run("kubectl version --client").future.get();
  LOG_INFO(logger, "Kubectl version: {}", kubectl_version);

  auto aws_version = run("aws --version").future.get();
  LOG_INFO(logger, "AWS version: {}", aws_version);

  auto eksctl_version = run("eksctl version").future.get();
  LOG_INFO(logger, "Eksctl version: {}", eksctl_version);
}
