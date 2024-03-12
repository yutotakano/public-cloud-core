#include "nv.h"
#include "subprocess_error.h"

NVApp::NVApp()
{
  logger = quill::get_logger();
  executor = Executor();
}

std::unique_ptr<argparse::ArgumentParser> NVApp::nv_arg_parser()
{
  auto parser = std::make_unique<argparse::ArgumentParser>("nv");
  parser->add_description("Pass commands to Nervion kubectl.");
  parser->add_argument("command").remaining();
  return parser;
}

void NVApp::nv_command_handler(argparse::ArgumentParser &parser)
{
  std::vector<std::string> command;
  try
  {
    command = parser.get<std::vector<std::string>>("command");
  }
  catch (std::logic_error &e)
  {
    LOG_ERROR(logger, "No command provided.");
    return;
  }

  run_kubectl(command).future.get();
}

ExecutingProcess
NVApp::run_kubectl(std::vector<std::string> command, bool stream_cout)
{
  command.insert(
    command.begin(),
    {"kubectl", "--kubeconfig", "config", "--context", "nervion-eks"}
  );
  return executor.run(command, stream_cout, false, false);
}
