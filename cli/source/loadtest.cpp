#include "loadtest.h"
#include "utils.h"
#include <cpr/cpr.h>
#include <json/json.hpp>

LoadTestApp::LoadTestApp()
{
  logger = quill::get_logger();
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

  std::string experiment_name =
    "experiment_" + parser.get<std::string>("--experiment-name");
  LOG_TRACE_L3(logger, "Experiment name: {}", experiment_name);

  // Check experiment destinations if we are collecting anything
  if (
    parser.get<bool>("--collect-avg-latency") ||
    parser.get<bool>("--collect-avg-throughput") ||
    parser.get<bool>("--collect-avg-cpu") ||
    parser.get<bool>("--collect-worker-count")
  )
  {
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

    // Create any parent directories for the files
    std::filesystem::path p = experiment_name;
    LOG_TRACE_L3(logger, "Experiment path prefix: {}", p);
    if (!std::filesystem::is_directory(p.parent_path()) && !p.parent_path().empty())
    {
      try
      {
        std::filesystem::create_directories(p.parent_path());
      }
      catch (const std::exception &e)
      {
        LOG_ERROR(logger, "Could not create directory for experiment files.");
        LOG_ERROR(logger, "Error: {}", e.what());
        return;
      }
    }
  }

  // Send the loadtest file to the Nervion controller
  try
  {
    post_nervion_controller(file_path, info.value(), minutes);
  }
  catch (const std::exception &e)
  {
    LOG_ERROR(logger, "Failed to start loadtest: {}", e.what());
    return;
  }
  LOG_INFO(logger, "Loadtest started.");

  // If any collection flags are present, collect data
  if (
    parser.get<bool>("--collect-avg-latency") ||
    parser.get<bool>("--collect-avg-throughput") ||
    parser.get<bool>("--collect-avg-cpu") ||
    parser.get<bool>("--collect-worker-count")
  )
  {
    // Perform collection every 5 seconds until 1000 seconds have passed
    int total_points = 200;
    LOG_TRACE_L3(logger, "Collecting 200 points of data.");
    for (int i = 0; i < total_points; i++)
    {
      LOG_INFO(
        logger,
        "Collecting data... {}/{} ({} seconds remaining)",
        i,
        total_points,
        (total_points * 5) - (i * 5)
      );
      if (parser.get<bool>("--collect-avg-latency"))
      {
        bool result = collect_avg_latency(
          i * 5,
          info->ck_prometheus_elb_url,
          experiment_name
        );
        // Latency doesn't get collected until the first UE is connected, and
        // there's no meaning to reducing timer before that point.
        if (!result)
        {
          LOG_INFO(
            logger,
            "No latency data available yet. Extending experiment."
          );
          total_points += 1;
        }
      }
      if (parser.get<bool>("--collect-worker-count"))
      {
        collect_worker_count(
          i * 5,
          info->ck_prometheus_elb_url,
          experiment_name
        );
      }
      if (parser.get<bool>("--collect-avg-throughput"))
      {
        collect_avg_throughput(
          i * 5,
          info->ck_prometheus_elb_url,
          experiment_name
        );
      }
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }
}

std::string
LoadTestApp::get_prometheus_value(std::string url, std::string query)
{
  // Collect data from the Prometheus server
  cpr::Response res = cpr::Get(
    cpr::Url{url + ":9090/api/v1/query"},
    cpr::Parameters{{"query", query}}
  );

  if (res.status_code != 200)
  {
    LOG_ERROR(logger, "cpr::Get() failed: {}", res.error.message);
    return "";
  }

  LOG_TRACE_L3(logger, "HTTP Request to Prometheus succeeded.");
  LOG_TRACE_L3(logger, "Response: {}", res.text);
  nlohmann::json stats = nlohmann::json::parse(res.text);
  if (stats["data"]["result"][0]["value"][1].is_null())
  {
    return "NaN";
  }

  return stats["data"]["result"][0]["value"][1].get<std::string>();
}

std::optional<int>
LoadTestApp::get_prometheus_value_int(std::string url, std::string query)
{
  std::string value = get_prometheus_value(url, query);
  if (value == "NaN")
  {
    return std::nullopt;
  }
  return std::stoi(value);
}

std::optional<float>
LoadTestApp::get_prometheus_value_float(std::string url, std::string query)
{
  std::string value = get_prometheus_value(url, query);
  if (value == "NaN")
  {
    return std::nullopt;
  }
  return std::stof(value);
}

bool LoadTestApp::collect_avg_latency(
  int time_since_start,
  std::string prometheus_url,
  std::string experiment_name
)
{
  LOG_TRACE_L3(logger, "Collecting average latency.");
  // Collect data from the Prometheus server
  std::optional<int> latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_latency{nas_type!=\"0\"})"
  );
  if (!latency.has_value())
  {
    LOG_TRACE_L3(logger, "Latency not available currently. Skipping.");
    return false;
  }

  std::optional<int> decode_latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_decode_latency{nas_type!=\"0\"})"
  );
  if (!decode_latency.has_value())
  {
    LOG_TRACE_L3(logger, "Decode latency not available currently. Assuming 0.");
    decode_latency = 0;
  }

  std::optional<int> handle_latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_handle_latency{nas_type!=\"0\"})"
  );
  if (!handle_latency.has_value())
  {
    LOG_TRACE_L3(logger, "Handle latency not available currently. Assuming 0.");
    handle_latency = 0;
  }

  std::optional<int> nas_decode_latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_nas_decode_latency{nas_type!=\"0\"})"
  );
  if (!nas_decode_latency.has_value())
  {
    LOG_TRACE_L3(
      logger,
      "NAS decode latency not available currently. Assuming 0."
    );
    nas_decode_latency = 0;
  }

  std::optional<int> nas_handle_latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_nas_handle_latency{nas_type!=\"0\"})"
  );
  if (!nas_handle_latency.has_value())
  {
    LOG_TRACE_L3(
      logger,
      "NAS Handle latency not available currently. Assuming 0."
    );
    nas_handle_latency = 0;
  }

  std::optional<int> nas_encode_latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_nas_encode_latency{nas_type!=\"0\"})"
  );
  if (!nas_encode_latency.has_value())
  {
    LOG_TRACE_L3(
      logger,
      "NAS Encode latency not available currently. Assuming 0."
    );
    nas_encode_latency = 0;
  }

  std::optional<int> build_latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_build_latency{nas_type!=\"0\"})"
  );
  if (!build_latency.has_value())
  {
    LOG_TRACE_L3(
      logger,
      "Response build latency not available currently. Assuming 0."
    );
    build_latency = 0;
  }

  std::optional<int> encode_latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_encode_latency{nas_type!=\"0\"})"
  );
  if (!encode_latency.has_value())
  {
    LOG_TRACE_L3(logger, "Encode latency not available currently. Assuming 0.");
    encode_latency = 0;
  }

  std::optional<int> send_latency = get_prometheus_value_int(
    prometheus_url,
    "avg(amf_message_send_latency{nas_type!=\"0\"})"
  );
  if (!send_latency.has_value())
  {
    LOG_TRACE_L3(logger, "Send latency not available currently. Assuming 0.");
    send_latency = 0;
  }

  // Write to file
  LOG_TRACE_L3(logger, "Writing results to _latency csv.");
  std::ofstream latency_file(
    experiment_name + "_latency.csv",
    std::ios::app | std::ios::out
  );
  latency_file << time_since_start << ", " << latency.value() << ", "
               << decode_latency.value() << ", " << handle_latency.value()
               << ", " << nas_decode_latency.value() << ", "
               << nas_handle_latency.value() << ", "
               << nas_encode_latency.value() << ", " << build_latency.value()
               << ", " << encode_latency.value() << ", " << send_latency.value()
               << std::endl;
  latency_file.close();
  if (!latency_file.good())
  {
    LOG_ERROR(
      logger,
      "Error writing to file {}: {}",
      experiment_name + "_latency.csv",
      strerror(errno)
    );
    return false;
  }
  LOG_TRACE_L3(logger, "Finished writing data.");

  return true;
}

bool LoadTestApp::collect_avg_throughput(
  int time_since_start,
  std::string prometheus_url,
  std::string experiment_name
)
{
  LOG_TRACE_L3(logger, "Collecting uplink throughput rate.");
  // Collect data from the Prometheus server
  float uplink_packets_rate =
    get_prometheus_value_float(
      prometheus_url,
      "rate(amf_uplink_packets_total[5s])"
    )
      .value_or(0);

  LOG_TRACE_L3(logger, "Collecting downlink throughput rate.");
  float downlink_packets_rate =
    get_prometheus_value_float(
      prometheus_url,
      "rate(amf_downlink_packets_total[5s])"
    )
      .value_or(0);

  // Write to file
  LOG_TRACE_L3(logger, "Writing results to _throughput csv.");
  std::ofstream throughput_file(
    experiment_name + "_throughput.csv",
    std::ios::app | std::ios::out
  );
  throughput_file << time_since_start << ", " << uplink_packets_rate << ", "
                  << downlink_packets_rate << std::endl;
  throughput_file.close();
  if (!throughput_file.good())
  {
    LOG_ERROR(
      logger,
      "Error writing to file {}: {}",
      experiment_name + "_throughput.csv",
      strerror(errno)
    );
    return false;
  }
  LOG_TRACE_L3(logger, "Finished writing data.");

  return true;
}

bool LoadTestApp::collect_worker_count(
  int time_since_start,
  std::string prometheus_url,
  std::string experiment_name
)
{
  LOG_TRACE_L3(logger, "Collecting worker count.");
  // Collect data from the Prometheus server
  int worker_count =
    get_prometheus_value_int(
      prometheus_url,
      "count(kube_pod_container_info{container=\"corekube-worker\"})"
    )
      .value_or(0);

  // Write to file
  LOG_TRACE_L3(logger, "Writing results to _worker_count csv.");
  std::ofstream worker_count_file(
    experiment_name + "_worker_count.csv",
    std::ios::app | std::ios::out
  );
  worker_count_file << time_since_start << ", " << worker_count << std::endl;
  worker_count_file.close();
  if (!worker_count_file.good())
  {
    LOG_ERROR(
      logger,
      "Error writing to file {}: {}",
      experiment_name + "_worker_count.csv",
      strerror(errno)
    );
    return false;
  }
  LOG_TRACE_L3(logger, "Finished writing data.");

  return true;
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
  LOG_TRACE_L3(logger, "URL: {}", url);

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
    },
    // Follow no redirects
    cpr::Redirect{0, false, false, cpr::PostRedirectFlags::NONE}
  );

  if (res.status_code == 200)
  {
    // No redirect, there was an error -- probably because there is already a
    // loadtest running
    LOG_ERROR(logger, "Starting loadtest failed, is there an existing run?");
    LOG_ERROR(logger, "Stop it with --stop and try again.");
    throw std::runtime_error("Failed to start loadtest.");
  }

  if (res.status_code != 302)
  {
    LOG_ERROR(logger, "cpr::Post() failed ({}): {}", res.status_code, res.text);
    if (res.status_code == 0)
    {
      LOG_ERROR(logger, "Could not conect: {}", res.error.message);
    }
    throw std::runtime_error("Failed to start loadtest.");
  }
}
