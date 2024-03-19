#include "loadtest.h"
#include "utils.h"
#include <cpr/cpr.h>
#include <json/json.hpp>

LoadTestApp::LoadTestApp()
{
  logger = quill::create_logger("loadtest");
  executor = Executor();
  ck_app = CKApp();
  nv_app = NVApp();
  info_app = InfoApp();
}

std::unique_ptr<argparse::ArgumentParser> LoadTestApp::loadtest_arg_parser()
{
  auto parser = std::make_unique<argparse::ArgumentParser>("loadtest");
  parser->add_description("Run a loadtest on the current deployment.");
  parser->add_argument("--file").help("Definition file for Nervion load");
  parser->add_argument("--incremental")
    .help("Duration of incremental scaling")
    .scan<'i', int>();
  parser->add_argument("--stop")
    .help("Stop the current loadtest")
    .default_value(false)
    .implicit_value(true);
  parser->add_argument("--collect-avg-latency")
    .help("Collect average latency during the loadtest into a file")
    .default_value(false)
    .implicit_value(true);
  parser->add_argument("--collect-avg-throughput")
    .help("Collect average throughput during the loadtest into a file")
    .default_value(false)
    .implicit_value(true);
  parser->add_argument("--collect-avg-cpu")
    .help("Collect average CPU usage during the loadtest into a file")
    .default_value(false)
    .implicit_value(true);
  parser->add_argument("--collect-worker-count")
    .help("Collect worker count during the loadtest into a file")
    .default_value(false)
    .implicit_value(true);
  parser->add_argument("--experiment-name")
    .help("Name of the experiment")
    .default_value(Utils::current_time());
  return parser;
}

void LoadTestApp::loadtest_command_handler(argparse::ArgumentParser &parser)
{
  LOG_DEBUG(logger, "Getting deployment info.");

  if (!info_app.check_contexts_exist())
  {
    LOG_ERROR(logger, "Could not run loadtest: no active deployment found.");
    return;
  }

  auto info = info_app.get_info();

  if (parser.get<bool>("--stop"))
  {
    LOG_INFO(logger, "Stopping the current loadtest.");
    // Stop the current loadtest
    auto completion_future = stop_nervion_controller(info.value());
    completion_future.get();
    return;
  }

  std::string file_path = parser.get<std::string>("--file");
  LOG_TRACE_L3(logger, "File: {}", file_path);
  LOG_INFO(logger, "Running loadtest with file: {}", file_path);

  int minutes = parser.present<int>("--incremental").value_or(0);
  LOG_TRACE_L3(logger, "Incremental duration: {}", minutes);

  // Send the loadtest file to the Nervion controller
  post_nervion_controller(file_path, info.value(), minutes);
  LOG_INFO(logger, "Loadtest started.");

  // If any collection flags are present, collect data
  if (
    parser.get<bool>("--collect-avg-latency") ||
    parser.get<bool>("--collect-avg-throughput") ||
    parser.get<bool>("--collect-avg-cpu") ||
    parser.get<bool>("--collect-worker-count")
  )
  {
    std::string experiment_name =
      "experiment_" + parser.get<std::string>("--experiment-name");
    // Abort if files already exist
    if ((std::filesystem::exists(experiment_name + "_latency.csv") ||
         std::filesystem::exists(experiment_name + "_throughput.csv") ||
         std::filesystem::exists(experiment_name + "_cpu.csv") ||
         std::filesystem::exists(experiment_name + "_worker_count.csv")))
    {
      LOG_ERROR(
        logger,
        "One or more files prefixed with '{}' exit. Aborting.",
        experiment_name
      );
      return;
    }

    // Perform collection every 5 seconds until 1000 seconds have passed
    int total_points = 200;
    for (int i = 0; i < total_points; i++)
    {
      LOG_INFO(
        logger,
        "Collecting data... {}/200 ({} seconds remaining)",
        i,
        1000 - (i * 5)
      );
      std::this_thread::sleep_for(std::chrono::seconds(5));
      collect_data(
        info.value(),
        i * 5,
        experiment_name,
        parser.get<bool>("--collect-avg-latency"),
        parser.get<bool>("--collect-avg-throughput"),
        parser.get<bool>("--collect-avg-cpu"),
        parser.get<bool>("--collect-worker-count")
      );
    }
  }
}

void LoadTestApp::collect_data(
  deployment_info_s info,
  int time_since_start,
  std::string experiment_name,
  bool collect_avg_latency,
  bool collect_avg_throughput,
  bool collect_avg_cpu,
  bool collect_worker_count
)
{
  if (collect_avg_latency)
  {
    // Collect data from the Prometheus server
    std::string url = info.ck_prometheus_elb_url + ":9090/api/v1/query";
    cpr::Response res = cpr::Get(
      cpr::Url{url},
      cpr::Parameters{{"query", "avg (amf_message_latency{nas_type!=\"0\"})"}}
    );

    if (res.status_code != 200)
    {
      LOG_ERROR(logger, "cpr::Get() failed: {}", res.error.message);
      return;
    }

    nlohmann::json stats = nlohmann::json::parse(res.text);
    int latency = stats["data"]["result"][0]["value"][1].get<int>();

    // Write to file
    std::ofstream latency_file(
      experiment_name + "_latency.csv",
      std::ios::app | std::ios::out
    );
    latency_file << time_since_start << ", " << latency << std::endl;
    latency_file.close();
  }

  if (collect_worker_count)
  {
    // Collect data from the Prometheus server
    std::string url = info.ck_prometheus_elb_url + ":9090/api/v1/query";
    cpr::Response res = cpr::Get(
      cpr::Url{url},
      cpr::Parameters{
        {"query",
         "count(kube_pod_container_info{container=\"corekube-worker\"})"}
      }
    );

    if (res.status_code != 200)
    {
      LOG_ERROR(logger, "cpr::Get() failed: {}", res.error.message);
      return;
    }

    nlohmann::json stats = nlohmann::json::parse(res.text);
    int worker_count = stats["data"]["result"][0]["value"][1].get<int>();

    // Write to file
    std::ofstream worker_count_file(
      experiment_name + "_worker_count.csv",
      std::ios::app | std::ios::out
    );
    worker_count_file << time_since_start << ", " << worker_count << std::endl;
    worker_count_file.close();
  }
  // if (collect_avg_cpu)
  // {
  //   std::ofstream cpu_file(
  //     experiment_name + "_cpu.csv",
  //     std::ios::app | std::ios::out
  //   );
  //   cpu_file << stats["cpu"] << std::endl;
  //   cpu_file.close();
  // }
}

std::future<void> LoadTestApp::stop_nervion_controller(deployment_info_s info)
{
  return std::async(
    [&, info]()
    {
      std::string url = info.nv_controller_elb_url + ":8080/restart/";
      cpr::Response res = cpr::Get(cpr::Url{url});

      if (res.status_code != 200)
      {
        LOG_ERROR(logger, "cpr::Get() failed: {}", res.error.message);

        // Manually delete all pods beginning with name 'slave-'
        LOG_INFO(
          logger,
          "Manually deleting pods assuming Nervion is in undefined state."
        );
        std::string pods_str =
          nv_app
            .run_kubectl(
              {"get", "pods", "--no-headers", "--output=name"},
              false
            )
            .future.get();
        auto pods = Utils::split(pods_str, '\n');
        for (auto &pod : pods)
        {
          if (pod.find("slave-") != std::string::npos)
          {
            LOG_DEBUG(logger, "Manually deleting pod: {}", pod);
            nv_app.run_kubectl({"delete", pod, "--wait=false"});
          }
        }
      }

      // Wait for all pods beginning with name 'slave-' to be deleted
      int last_pod_count = 0;
      while (true)
      {
        std::string pods_str =
          nv_app.run_kubectl({"get", "pods", "--no-headers"}, false)
            .future.get();
        auto pods = Utils::split(pods_str, '\n');
        int num_slaves = 0;
        for (auto &pod : pods)
        {
          if (pod.find("slave-") != std::string::npos)
            num_slaves++;
        }
        if (num_slaves == 0)
          break;

        if (num_slaves != last_pod_count)
        {
          LOG_INFO(logger, "Waiting for {} pods to be deleted.", num_slaves);
          last_pod_count = num_slaves;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
      }

      LOG_INFO(logger, "Loadtest stopped.");
    }
  );
}

void LoadTestApp::post_nervion_controller(
  std::string file_path,
  deployment_info_s info,
  int incremental_duration
)
{
  // Send the file to the Nervion controller via libcurl

  // Get last part of file path to get the file name
  std::string file_name = Utils::split(file_path, '/').back();

  std::string url = info.nv_controller_elb_url + ":8080/config/";
  cpr::Response res = cpr::Post(
    cpr::Url{url},
    cpr::Multipart{
      {"mme_ip", info.ck_frontend_ip},
      {"cp_mode", "true"},
      {"threads", "100"},
      {"scale_minutes", std::to_string(incremental_duration)},
      {"multi_ip", ""},
      {"config", cpr::File{file_path, file_name}},
      {"docker_image", "ghcr.io/yutotakano/ran-slave:latest"},
      {"refresh_time", "10"}
    }
  );

  if (res.status_code != 200)
  {
    LOG_ERROR(logger, "cpr::Post() failed: {}", res.error.message);
  }
}
