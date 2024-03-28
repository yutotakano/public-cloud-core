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

  /**
   * @brief Query the Prometheus HTTP API to get the value for a PromQL query
  /// as a string.
   *
   * @param prometheus_url
   * @param query
   * @return std::string
   */
  std::string
  get_prometheus_value(std::string prometheus_url, std::string query);

  /**
   * @brief Get the value of a PromQL query as an integer, handling NaN as
   * nullopt.
   *
   * @param prometheus_url
   * @param query
   * @return std::optional<int>
   */
  std::optional<int>
  get_prometheus_value_int(std::string prometheus_url, std::string query);

  /**
   * @brief Get the value of a PromQL query as a float, handling NaN as nullopt.
   *
   * @param prometheus_url
   * @param query
   * @return std::optional<float>
   */
  std::optional<float>
  get_prometheus_value_float(std::string prometheus_url, std::string query);

  /**
   * @brief Start a Nervion controller load test using the specified workflow
   * file.
   *
   * @param file_path
   * @param info
   * @param incremental_duration
   */
  void post_nervion_controller(
    std::string file_path,
    deployment_info_s info,
    int incremental_duration
  );

  /**
   * @brief Query the Prometheus API for the average latency seen very recently.
   *
   * @param time_since_start
   * @param prometheus_url
   * @return std::vector<std::pair<std::string, std::string>> A vector of data
   * name and value pairs. Empty if latency is not available.
   */
  std::vector<std::pair<std::string, std::string>>
  collect_avg_latency(int time_since_start, std::string prometheus_url);

  /**
   * @brief Query the Prometheus API for the number of worker pods seen very
   * recently.
   *
   * @param time_since_start
   * @param prometheus_url
   * @return std::vector<std::pair<std::string, std::string>> A vector of data
   * name and value pairs.
   */
  std::vector<std::pair<std::string, std::string>>
  collect_worker_count(int time_since_start, std::string prometheus_url);

  /**
   * @brief  Query the Prometheus API for the average network throughput as
   * averaged as a rate per second based on the last 5 seconds of data.
   *
   * @param time_since_start
   * @param prometheus_url
   * @return std::vector<std::pair<std::string, std::string>> A vector of data
   * name and value pairs.
   */
  std::vector<std::pair<std::string, std::string>>
  collect_avg_throughput(int time_since_start, std::string prometheus_url);

  /**
   * @brief Query the KubeCost Prometheus server for the cost of the cluster.
   *
   * @param time_since_start
   * @param prometheus_url
   * @return std::vector<std::pair<std::string, std::string>> A vector of data
   * name and value pairs.
   */
  std::vector<std::pair<std::string, std::string>>
  collect_cost(int time_since_start, std::string prometheus_url);

  bool write_to_csv(
    std::string file_name,
    std::vector<std::pair<std::string, std::string>> data
  );

  /**
   * @brief Cause an anomaly in the core.
   *
   * @param frontend_ip
   * @param type 0x03 for crash, 0x04 for CPU spike.
   */
  void cause_anomaly(std::string frontend_ip, uint8_t type);

  quill::Logger *logger;
  Executor executor;
  InfoApp info_app;
  CKApp ck_app;
  NVApp nv_app;
};

#endif // LOADTEST_H
