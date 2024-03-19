#ifndef INFO_H
#define INFO_H

#include "argparse/argparse.hpp"
#include "ck.h"
#include "executor.h"
#include "nv.h"
#include "quill/Quill.h"

struct deployment_info_s
{
  std::string ck_grafana_elb_url;
  std::string ck_prometheus_elb_url;
  std::string ck_opencost_elb_url;
  std::string nv_controller_elb_url;
  std::string ck_frontend_ip;
};

class InfoApp
{
public:
  InfoApp();

  std::unique_ptr<argparse::ArgumentParser> info_arg_parser();

  void info_command_handler(argparse::ArgumentParser &parser);

  bool check_contexts_exist();
  std::optional<deployment_info_s> get_info();

private:
  quill::Logger *logger;
  Executor executor;
  CKApp ck_app;
  NVApp nv_app;
};

#endif // INFO_H
