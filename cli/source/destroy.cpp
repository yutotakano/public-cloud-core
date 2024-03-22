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
  // Determine the value of the deployment_type variable used in the Terraform
  // state
  LOG_TRACE_L3(logger, "Autodetecting deployment type...");
  std::string deployment_type;
  try
  {
    deployment_type =
      executor
        .run(
          {"terraform",
           "-chdir=terraform/base",
           "output",
           "-raw",
           "deployment_type"}
        , false, true, true)
        .future.get();
  }
  catch (const SubprocessError &e)
  {
    LOG_ERROR(
      logger,
      "Error getting deployment type from Terraform state: {}",
      e.what()
    );
    LOG_ERROR(logger, "Is there an active deployment?");
    return;
  }

  LOG_INFO(logger, "Detected deployment type: {}", deployment_type);

  if (deployment_type == "fargate")
  {
    teardown_aws_eks_fargate();
  }
  else if (deployment_type == "ec2")
  {
    teardown_aws_eks_ec2();
  }
  else
  {
    LOG_ERROR(
      logger,
      "Unknown deployment type in existing Terraform state: {}",
      deployment_type
    );
  }
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
    .run(
      {"terraform",
       "-chdir=terraform/base",
       "destroy",
       "-auto-approve",
       "-var",
       "deployment_type=fargate"}
    )
    .future.get();

  LOG_INFO(logger, "CoreKube tore down successfully!");
}

void DestroyApp::teardown_aws_eks_ec2()
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
    .run(
      {"terraform",
       "-chdir=terraform/base",
       "destroy",
       "-auto-approve",
       "-var",
       "deployment_type=ec2"}
    )
    .future.get();

  LOG_INFO(logger, "CoreKube tore down successfully!");
}

void DestroyApp::teardown_aws_eks_ec2_spot() { return; }

void DestroyApp::teardown_aws_ec2() { return; }
