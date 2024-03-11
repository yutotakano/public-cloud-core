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

struct context_info_s
{
  std::string current_context;
  std::string corekube_context;
  std::string nervion_context;
};

class InfoApp
{
public:
  InfoApp();

  std::unique_ptr<argparse::ArgumentParser> info_arg_parser();

  void info_command_handler(argparse::ArgumentParser &parser);

  void switch_context(std::string context);
  std::optional<context_info_s> get_contexts();
  std::optional<deployment_info_s> get_info();

private:
  quill::Logger *logger;
  Executor executor;
};

#endif // INFO_H
