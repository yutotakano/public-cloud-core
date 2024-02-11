#include "deploy.h"
#include "argparse/argparse.hpp"
#include "quill/Quill.h"
#include "subprocess/subprocess.hpp"
#include "trace.h"

DeployApp::DeployApp() { logger = quill::get_logger(); }

std::unique_ptr<argparse::ArgumentParser> DeployApp::deploy_arg_parser()
{
  auto deploy_command = std::make_unique<argparse::ArgumentParser>("deploy");
  deploy_command->add_description("CLI to deploy CoreKube on the public cloud");

  deploy_command->add_argument("--type")
    .default_value(std::string{"aws-eks-fargate"})
    .help("Type of deployment")
    .choices("aws-eks-fargate", "aws-eks-ec2", "aws-eks-ec2-spot", "aws-ec2");

  deploy_command->add_argument("--ssh-key")
    .default_value(std::string{""})
    .help("SSH key to use for the deployment");

  return deploy_command;
}

void DeployApp::deploy_command_handler(argparse::ArgumentParser &parser)
{
  TRACE_FUNCTION_ENTER(logger);

  LOG_INFO(logger, "Deploying CoreKube on the public cloud...");

  subprocess::Popen popen =
    subprocess::RunBuilder({"/bin/bash", "-c", "./scripts/setup_fargate.sh"})
      .cout(subprocess::PipeOption::pipe)
      .popen();
  char buffer[1024];
  while (popen.poll() == false)
  {
    subprocess::pipe_read(popen.cout, buffer, 1024);
    std::cout << buffer;
  }

  LOG_INFO(logger, "CoreKube deployed successfully!");

exit:
  TRACE_FUNCTION_EXIT(logger);
}
