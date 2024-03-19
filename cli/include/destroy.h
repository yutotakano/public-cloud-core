#ifndef DESTROY_H
#define DESTROY_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "info.h"
#include "quill/Quill.h"

class DestroyApp
{
public:
  DestroyApp();

  std::unique_ptr<argparse::ArgumentParser> arg_parser();

  void command_handler(argparse::ArgumentParser &parser);

  void teardown_autodetect();

private:
  void teardown_aws_eks_fargate();
  void teardown_aws_eks_ec2();
  void teardown_aws_eks_ec2_spot();
  void teardown_aws_ec2();
  quill::Logger *logger;
  Executor executor;
  InfoApp info_app;
};

#endif // DESTROY_H
