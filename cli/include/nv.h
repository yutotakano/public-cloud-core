#ifndef NV_H
#define NV_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"

class NVApp
{
public:
  NVApp();

  std::unique_ptr<argparse::ArgumentParser> nv_arg_parser();

  void nv_command_handler(argparse::ArgumentParser &parser);
  ExecutingProcess
  run_kubectl(std::vector<std::string> command, bool stream_cout = true);

private:
  quill::Logger *logger;
  Executor executor;
};

#endif // NV_H
