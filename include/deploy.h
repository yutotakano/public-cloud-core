#ifndef DEPLOY_H
#define DEPLOY_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"

class DeployApp
{
public:
  DeployApp();

  std::unique_ptr<argparse::ArgumentParser> deploy_arg_parser();
  void deploy_command_handler(argparse::ArgumentParser &parser);

  void deploy_aws_eks_fargate();
  void deploy_aws_eks_ec2();
  void deploy_aws_eks_ec2_spot();
  void deploy_aws_ec2();

private:
  quill::Logger *logger;
  Executor executor;
};

#endif
