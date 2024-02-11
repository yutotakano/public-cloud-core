#include "argparse.hpp"
#include "deploy.h"

std::unique_ptr<argparse::ArgumentParser> deploy_arg_parser()
{
  auto deploy_command = std::make_unique<argparse::ArgumentParser>("deploy");
  deploy_command->add_description(
    "Deploy CoreKube on the public cloud"
  );
  deploy_command->add_argument("--type")
    .default_value(std::string{"aws-eks-fargate"})
    .help("Type of deployment")
    .choices(
      "aws-eks-fargate",
      "aws-eks-ec2",
      "aws-eks-ec2-spot",
      "aws-ec2"
    );
  deploy_command->add_argument("--ssh-key")
    .default_value(std::string{""})
    .help("SSH key to use for the deployment");

  return deploy_command;
}
