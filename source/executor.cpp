#include "executor.h"
#include "quill/Quill.h"
#include "subprocess/subprocess.h"
#include "trace.h"
#include "utils.h"
#include <future>
#include <iostream>
#include <string>

Executor::Executor() { logger = quill::get_logger(); }

ExecutingProcess Executor::run(std::string command)
{
  TRACE_FUNCTION_ENTER(logger);

  LOG_DEBUG(logger, "Running command: {}", command);

  // Find shell (bash or cmd.exe) and command prefix (-c or /c)
  // to use for running the command -- this allows us to run commands including
  // pipes and other shell features without having to worry *too much* about the
  // differences between bash and cmd.exe
  char *shell = "/bin/bash";
  char *command_prefix;
  if (char *env_shell = std::getenv("SHELL"))
  {
    shell = env_shell;
    command_prefix = "-c";
  }
  else if (char *env_comspec = std::getenv("COMSPEC"))
  {
    shell = env_comspec;
    command_prefix = "/c";
  }

  LOG_DEBUG(logger, "Raw command: \"{}\"", command);

  std::vector<std::string> command_parts = Utils::split(command, ' ');
  std::vector<char *> cstr_command_parts;
  cstr_command_parts.reserve(command_parts.size());

  for (size_t i = 0; i < command_parts.size(); ++i)
  {
    cstr_command_parts.push_back(const_cast<char *>(command_parts[i].c_str()));
  }
  cstr_command_parts.push_back(nullptr);

  std::shared_ptr<subprocess_s> process = std::make_shared<subprocess_s>();
  int process_result = subprocess_create(
    &cstr_command_parts[0],
    subprocess_option_enable_async | subprocess_option_inherit_environment,
    process.get()
  );
  if (process_result != 0)
  {
    LOG_ERROR(logger, "Error creating subprocess: {}", process_result);
  }

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

        LOG_TRACE_L3(logger, "Reading from process stdout");
        bread = subprocess_read_stdout(process.get(), out_buffer, 1024);
        LOG_TRACE_L3(logger, "{} bytes read", bread);
        LOG_TRACE_L3(logger, "Output: {}", out_buffer);

        output += out_buffer;

        LOG_TRACE_L3(logger, "Reading from process stderr");
        bread = subprocess_read_stderr(process.get(), err_buffer, 1024);
        LOG_TRACE_L3(logger, "{} bytes read", bread);
        LOG_TRACE_L3(logger, "Output: {}", err_buffer);

        error += err_buffer;
      }
      LOG_ERROR(logger, "Error: {}", error);

      return output;
    }
  );

  TRACE_FUNCTION_EXIT(logger);
  return result;
}
