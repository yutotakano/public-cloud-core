#ifndef DEPLOY_H
#define DEPLOY_H

#include "argparse/argparse.hpp"
#include "quill/Quill.h"

class DeployApp
{
public:
  DeployApp();

  std::unique_ptr<argparse::ArgumentParser> deploy_arg_parser();
  void deploy_command_handler(argparse::ArgumentParser &parser);

private:
  quill::Logger *logger;
};

#endif
