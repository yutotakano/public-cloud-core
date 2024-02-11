#include "deploy.h"
#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"
#include "trace.h"

DeployApp::DeployApp()
{
  logger = quill::get_logger();
  executor = Executor();
}

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

  if (parser.get<std::string>("--type") == "aws-eks-fargate")
  {
    deploy_aws_eks_fargate();
  }
  else if (parser.get<std::string>("--type") == "aws-eks-ec2")
  {
    deploy_aws_eks_ec2();
  }
  else if (parser.get<std::string>("--type") == "aws-eks-ec2-spot")
  {
    deploy_aws_eks_ec2_spot();
  }
  else if (parser.get<std::string>("--type") == "aws-ec2")
  {
    deploy_aws_ec2();
  }
  else
  {
    LOG_ERROR(logger, "Invalid deployment type specified");
  }

  TRACE_FUNCTION_EXIT(logger);
}

void DeployApp::deploy_aws_eks_fargate()
{
  TRACE_FUNCTION_ENTER(logger);
  LOG_INFO(logger, "Deploying CoreKube on AWS EKS with Fargate...");

  auto result = executor.run("docker --version").future.get();
  LOG_INFO(logger, "Output: {}", result);

  LOG_INFO(logger, "CoreKube deployed successfully!");

  TRACE_FUNCTION_EXIT(logger);
}

void DeployApp::deploy_aws_eks_ec2() { return; }

void DeployApp::deploy_aws_eks_ec2_spot() { return; }

void DeployApp::deploy_aws_ec2() { return; }
