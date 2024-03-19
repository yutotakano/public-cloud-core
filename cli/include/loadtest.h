#ifndef LOADTEST_H
#define LOADTEST_H

#include "argparse/argparse.hpp"
#include "ck.h"
#include "executor.h"
#include "info.h"
#include "nv.h"
#include "quill/Quill.h"

class LoadTestApp
{
public:
  LoadTestApp();

  std::unique_ptr<argparse::ArgumentParser> loadtest_arg_parser();

  void loadtest_command_handler(argparse::ArgumentParser &parser);

private:
  std::future<void> stop_nervion_controller(deployment_info_s info);
  void post_nervion_controller(
    std::string file_path,
    deployment_info_s info,
    int incremental_duration
  );

  void collect_avg_latency(
    int time_since_start,
    std::string prometheus_url,
    std::string experiment_name
  );

  void collect_worker_count(
    int time_since_start,
    std::string prometheus_url,
    std::string experiment_name
  );

  void collect_avg_throughput(
    int time_since_start,
    std::string prometheus_url,
    std::string experiment_name
  );

  quill::Logger *logger;
  Executor executor;
  InfoApp info_app;
  CKApp ck_app;
  NVApp nv_app;
};

#endif // LOADTEST_H
