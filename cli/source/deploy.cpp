#include "deploy.h"
#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"
#include "subprocess_error.h"
#include "utils.h"

DeployApp::DeployApp()
{
  logger = quill::get_logger();
  executor = Executor();
  info_app = InfoApp();
  destroy_app = DestroyApp();
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

  deploy_command->add_argument("--versions")
    .default_value(false)
    .implicit_value(true)
    .help("Print the versions of the tools used by the executor");

  deploy_command->add_argument("--teardown")
    .default_value(false)
    .implicit_value(true)
    .help("Teardown the previous deployment before deploying");

  deploy_command->add_argument("--no-deploy")
    .default_value(false)
    .implicit_value(true)
    .help("Don't deploy anything, just teardown the previous deployment");

  return deploy_command;
}

void DeployApp::deploy_command_handler(argparse::ArgumentParser &parser)
{
  if (parser.get<bool>("--versions"))
  {
    LOG_TRACE_L3(logger, "Printing tool versions");
    executor.print_versions();
    return;
  }

  bool teardown = parser.get<bool>("--teardown");
  bool no_setup = parser.get<bool>("--no-deploy");

  LOG_TRACE_L3(logger, "Teardown: {}", teardown);
  LOG_TRACE_L3(logger, "No setup: {}", no_setup);
  LOG_TRACE_L3(logger, "Type: {}", parser.get<std::string>("--type"));

  if (teardown)
  {
    destroy_app.teardown_autodetect();
  }
  if (no_setup)
    return;

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

  executor.run({"terraform", "-chdir=terraform/base", "apply", "-auto-approve"})
    .future.get();

  // Register the two clusters with the local kubeconfig
  executor
    .run(
      {"aws",
       "eks",
       "update-kubeconfig",
       "--kubeconfig",
       "config",
       "--name",
       "corekube-eks",
       "--alias",
       "corekube-eks"}
    )
    .future.get();
  executor
    .run(
      {"aws",
       "eks",
       "update-kubeconfig",
       "--kubeconfig",
       "config",
       "--name",
       "nervion-eks",
       "--alias",
       "nervion-eks"}
    )
    .future.get();

  executor
    .run({"terraform", "-chdir=terraform/kubernetes", "apply", "-auto-approve"})
    .future.get();

  // Get info
  auto info = info_app.get_info();

  LOG_INFO(logger, "CoreKube deployed successfully!");
  LOG_INFO(
    logger,
    "IP (within VPC) of CK frontend node: {}",
    info->ck_frontend_ip
  );
  LOG_INFO(logger, "Grafana: http://{}:3000", info->ck_grafana_elb_url);
  LOG_INFO(logger, "Nervion: http://{}:8080", info->nv_controller_elb_url);
  LOG_INFO(logger, "Next: publicore loadtest --file <file_path>");
}

void DeployApp::deploy_aws_eks_ec2() { return; }

void DeployApp::deploy_aws_eks_ec2_spot() { return; }

void DeployApp::deploy_aws_ec2() { return; }
