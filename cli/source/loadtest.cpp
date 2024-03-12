#include "loadtest.h"
#include "curl/curl.h"
#include "utils.h"

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
}

std::future<void> LoadTestApp::stop_nervion_controller(deployment_info_s info)
{
  return std::async(
    [&, info]()
    {
      // Send the file to the Nervion controller via libcurl
      CURL *curl = curl_easy_init();
      if (!curl)
        throw std::runtime_error("Could not initialize curl.");

      curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

      std::string url = info.nervion_dns_name + "/restart/";
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_PORT, 8080);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

      std::string reponse;
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reponse);

      CURLcode res = curl_easy_perform(curl);

      if (res != CURLE_OK)
      {
        LOG_ERROR(
          logger,
          "curl_easy_perform() failed: {}",
          curl_easy_strerror(res)
        );

        // Manually delete all pods beginning with name 'slave-'
        LOG_INFO(
          logger,
          "Manually deleting pods assuming Nervion is in undefined state."
        );
        std::string pods_str =
          nv_app.run_kubectl({"get", "pods", "--no-headers", "--output=name"})
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

      curl_easy_cleanup(curl);

      // Wait for all pods beginning with name 'slave-' to be deleted
      int last_pod_count = 0;
      while (true)
      {
        std::string pods_str =
          nv_app.run_kubectl({"get", "pods", "--no-headers"}).future.get();
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

size_t LoadTestApp::curl_write_callback(
  void *buffer,
  size_t size,
  size_t count,
  void *user
)
{
  size_t numBytes = size * count;
  static_cast<std::string *>(user)
    ->append(static_cast<char *>(buffer), 0, numBytes);
  return numBytes;
}

void LoadTestApp::post_nervion_controller(
  std::string file_path,
  deployment_info_s info,
  int incremental_duration
)
{
  // Send the file to the Nervion controller via libcurl
  CURL *curl = curl_easy_init();
  if (!curl)
    throw std::runtime_error("Could not initialize curl.");

  std::string file_name = Utils::split(file_path, '/').back();

  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  curl_mime *form = curl_mime_init(curl);
  curl_mimepart *field;

  field = curl_mime_addpart(form);
  curl_mime_name(field, "mme_ip");
  curl_mime_data(field, info.frontend_ip.c_str(), CURL_ZERO_TERMINATED);

  field = curl_mime_addpart(form);
  curl_mime_name(field, "cp_mode");
  curl_mime_data(field, "true", CURL_ZERO_TERMINATED);

  field = curl_mime_addpart(form);
  curl_mime_name(field, "threads");
  curl_mime_data(field, "100", CURL_ZERO_TERMINATED);

  field = curl_mime_addpart(form);
  curl_mime_name(field, "scale_minutes");
  curl_mime_data(
    field,
    std::to_string(incremental_duration).c_str(),
    CURL_ZERO_TERMINATED
  );

  field = curl_mime_addpart(form);
  curl_mime_name(field, "multi_ip");
  curl_mime_data(field, "", CURL_ZERO_TERMINATED);

  field = curl_mime_addpart(form);
  curl_mime_name(field, "config");
  curl_mime_filedata(field, file_path.c_str());

  field = curl_mime_addpart(form);
  curl_mime_name(field, "docker_image");
  curl_mime_data(
    field,
    "ghcr.io/yutotakano/ran-slave:latest",
    CURL_ZERO_TERMINATED
  );

  field = curl_mime_addpart(form);
  curl_mime_name(field, "refresh_time");
  curl_mime_data(field, "10", CURL_ZERO_TERMINATED);

  curl_slist *headerlist = curl_slist_append(NULL, "Expect:");

  std::string url = info.nervion_dns_name + "/config/";
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_PORT, 8080);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

  std::string reponse;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reponse);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK)
  {
    LOG_ERROR(
      logger,
      "curl_easy_perform() failed: {}",
      curl_easy_strerror(res)
    );
  }

  curl_easy_cleanup(curl);
  curl_mime_free(form);
  curl_slist_free_all(headerlist);
}
