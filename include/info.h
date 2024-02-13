#ifndef INFO_H
#define INFO_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"

struct deployment_info_s
{
  std::string corekube_dns_name;
  std::string nervion_dns_name;
  std::string frontend_ip;
};

class InfoApp
{
public:
  InfoApp();

  std::unique_ptr<argparse::ArgumentParser> info_arg_parser();

  void info_command_handler(argparse::ArgumentParser &parser);

  std::optional<deployment_info_s> get_info();

private:
  quill::Logger *logger;
  Executor executor;
};

#endif // INFO_H
