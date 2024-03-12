#ifndef CK_H
#define CK_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"

class CKApp
{
public:
  CKApp();

  std::unique_ptr<argparse::ArgumentParser> ck_arg_parser();

  void ck_command_handler(argparse::ArgumentParser &parser);
  ExecutingProcess
  run_kubectl(std::vector<std::string> command, bool stream_cout = true);

private:
  quill::Logger *logger;
  Executor executor;
};

#endif // CK_H
