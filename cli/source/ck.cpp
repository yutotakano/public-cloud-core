#include "ck.h"
#include "subprocess_error.h"

CKApp::CKApp()
{
  logger = quill::get_logger();
  executor = Executor();
}

std::unique_ptr<argparse::ArgumentParser> CKApp::ck_arg_parser()
{
  auto parser = std::make_unique<argparse::ArgumentParser>("ck");
  parser->add_description("Pass commands to CoreKube kubectl.");
  parser->add_argument("command").remaining();
  return parser;
}

void CKApp::ck_command_handler(argparse::ArgumentParser &parser)
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
CKApp::run_kubectl(std::vector<std::string> command, bool stream_cout)
{
  command.insert(
    command.begin(),
    {"kubectl", "--kubeconfig", "config", "--context", "corekube-eks"}
  );
  return executor.run(command, stream_cout, false, false);
}
