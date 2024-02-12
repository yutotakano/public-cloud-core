#include "deploy.h"
#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"

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
  LOG_INFO(logger, "Deploying CoreKube on the public cloud...");

  if (parser.get<std::string>("--type") == "aws-eks-fargate")
  {
    std::string public_key_path = parser.get<std::string>("--ssh-key");
    if (public_key_path.empty())
      public_key_path = get_public_key_path();

    deploy_aws_eks_fargate(public_key_path);
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
}

std::string DeployApp::get_public_key_path()
{
  char *home = std::getenv("HOME");
#ifdef _WIN32
  home = std::getenv("USERPROFILE");
#endif

  // Make sure getenv didn't return a null pointer
  std::string homedir = home ? std::string(home) : ".";

  LOG_TRACE_L3(logger, "User home directory: {}", homedir);

  std::vector<std::string> public_key_paths = {
    homedir + "/.ssh/id_rsa.pub",
    homedir + "/.ssh/id_dsa.pub",
    homedir + "/.ssh/id_ecdsa.pub",
    homedir + "/.ssh/id_ed25519.pub",
  };

  for (auto &path : public_key_paths)
  {
    LOG_TRACE_L3(logger, "Checking for public key at {}", path);
    if (std::filesystem::exists(path))
    {
      return path;
    }
  }

  LOG_ERROR(logger, "No public key found in $HOME/.ssh");
  return "";
}

void DeployApp::deploy_aws_eks_fargate(std::string public_key_path)
{
  LOG_INFO(logger, "Deploying CoreKube on AWS EKS with Fargate...");

  executor.print_versions();

  executor
    .run(
      "eksctl create cluster --name corekube-aws-cluster --region eu-north-1 "
      "--version 1.28 --without-nodegroup --node-private-networking "
      "--ssh-access --ssh-public-key " +
      public_key_path
    )
    .future.get();

  executor
    .run(
      "eksctl create nodegroup --cluster corekube-aws-cluster --name "
      "ng-corekube --node-ami-family AmazonLinux2 --node-type t3.small --nodes "
      "1 --nodes-min 1 --nodes-max 1 --node-volume-size 20 --ssh-access "
      "--ssh-public-key " +
      public_key_path + " --managed --asg-access"
    )
    .future.get();

  LOG_INFO(logger, "CoreKube deployed successfully!");
}

void DeployApp::deploy_aws_eks_ec2() { return; }

void DeployApp::deploy_aws_eks_ec2_spot() { return; }

void DeployApp::deploy_aws_ec2() { return; }
