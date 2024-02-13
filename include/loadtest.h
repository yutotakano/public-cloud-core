#ifndef LOADTEST_H
#define LOADTEST_H

#include "argparse/argparse.hpp"
#include "executor.h"
#include "info.h"
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
  void post_nervion_controller(std::string file_path, deployment_info_s info);

  quill::Logger *logger;
  Executor executor;
};

#endif // LOADTEST_H
