#ifndef ENV_H
#define ENV_H

#include "argparse/argparse.hpp"
#include "info.h"
#include "quill/Quill.h"

class EnvApp
{
public:
  EnvApp();

  std::unique_ptr<argparse::ArgumentParser> env_arg_parser();

  void env_command_handler(argparse::ArgumentParser &parser);

private:
  void switch_context(std::string context);
  void get_environment();

  quill::Logger *logger;
  InfoApp info_app;
};

#endif // ENV_H
