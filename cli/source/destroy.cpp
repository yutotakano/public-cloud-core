#include "destroy.h"
#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"
#include "subprocess_error.h"
#include "utils.h"

DestroyApp::DestroyApp()
{
  logger = quill::get_logger();
  executor = Executor();
  info_app = InfoApp();
}

std::unique_ptr<argparse::ArgumentParser> DestroyApp::arg_parser()
{
  auto destroy_command = std::make_unique<argparse::ArgumentParser>("destroy");
  destroy_command->add_description(
    "CLI to destroy an existing CoreKube instance"
  );

  return destroy_command;
}

void DestroyApp::command_handler(argparse::ArgumentParser &parser)
{
  teardown_autodetect();
}

void DestroyApp::teardown_autodetect()
{
  // TODO: auto-detect type from a cache or database of some sort
  // For now, assume EKS Fargate
  teardown_aws_eks_fargate();
}

void DestroyApp::teardown_aws_eks_fargate()
{
  LOG_INFO(logger, "Tearing down CoreKube on AWS EKS with Fargate...");

  // Destroy the kubernetes layer first, since the LoadBalancers in this layer
  // will directly prevent the base layer VPCs from being destroyed
  executor
    .run(
      {"terraform", "-chdir=terraform/kubernetes", "destroy", "-auto-approve"}
    )
    .future.get();

  executor
    .run({"terraform", "-chdir=terraform/base", "destroy", "-auto-approve"})
    .future.get();

  LOG_INFO(logger, "CoreKube tore down successfully!");
}

void DestroyApp::teardown_aws_eks_ec2() { return; }

void DestroyApp::teardown_aws_eks_ec2_spot() { return; }

void DestroyApp::teardown_aws_ec2() { return; }
