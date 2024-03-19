#ifndef DEPLOY_H
#define DEPLOY_H

#include "argparse/argparse.hpp"
#include "destroy.h"
#include "executor.h"
#include "info.h"
#include "quill/Quill.h"

class DeployApp
{
public:
  DeployApp();

  std::unique_ptr<argparse::ArgumentParser> deploy_arg_parser();

  void deploy_command_handler(argparse::ArgumentParser &parser);

private:
  std::string get_public_key_path();

  void deploy_aws_eks_fargate(std::string public_key_path);
  void deploy_aws_eks_ec2();
  void deploy_aws_eks_ec2_spot();
  void deploy_aws_ec2();

  quill::Logger *logger;
  Executor executor;
  InfoApp info_app;
  DestroyApp destroy_app;
};

#endif // DEPLOY_H
