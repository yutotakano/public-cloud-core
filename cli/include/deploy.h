#ifndef DEPLOY_H
#define DEPLOY_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"
#include <taskflow/taskflow.hpp>

class DeployApp
{
public:
  DeployApp();

  std::unique_ptr<argparse::ArgumentParser> deploy_arg_parser();

  void deploy_command_handler(argparse::ArgumentParser &parser);

private:
  std::string get_public_key_path();

  void teardown_aws_eks_fargate();
  void teardown_aws_eks_ec2();
  void teardown_aws_eks_ec2_spot();
  void teardown_aws_ec2();

  void deploy_aws_eks_fargate(std::string public_key_path);
  void deploy_aws_eks_ec2();
  void deploy_aws_eks_ec2_spot();
  void deploy_aws_ec2();

  std::string get_most_recent_cloudformation_event(std::string stack_name);
  std::future<void> eksctl_deletion_details(std::string stack_name);
  std::future<std::string> eksctl_delete_resource(
    std::string resource_type,
    std::vector<std::string> args
  );

  tf::Executor tf_executor;
  tf::Taskflow tf_taskflow;

  quill::Logger *logger;
  Executor executor;
};

#endif // DEPLOY_H