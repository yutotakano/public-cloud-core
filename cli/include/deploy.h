#ifndef DEPLOY_H
#define DEPLOY_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "info.h"
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

  tf::Executor tf_executor;
  tf::Taskflow tf_taskflow;

  quill::Logger *logger;
  Executor executor;
  InfoApp info_app;
};

#endif // DEPLOY_H
