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
  static size_t
  curl_write_callback(void *buffer, size_t size, size_t count, void *user);
  std::future<void> stop_nervion_controller(deployment_info_s info);
  void post_nervion_controller(
    std::string file_path,
    deployment_info_s info,
    int incremental_duration
  );

  void collect_data(
    deployment_info_s info,
  int time_since_start,
    std::string experiment_name,
    bool collect_avg_latency,
    bool collect_avg_throughput,
    bool collect_avg_cpu,
    bool collect_worker_count
  );

  quill::Logger *logger;
  Executor executor;
  InfoApp info_app;
  CKApp ck_app;
  NVApp nv_app;
};

#endif // LOADTEST_H
