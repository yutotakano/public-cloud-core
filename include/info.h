#ifndef INFO_H
#define INFO_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"

class InfoApp
{
public:
  InfoApp();

  std::unique_ptr<argparse::ArgumentParser> info_arg_parser();

  void info_command_handler(argparse::ArgumentParser &parser);

private:
  quill::Logger *logger;
  Executor executor;
};

#endif // INFO_H
