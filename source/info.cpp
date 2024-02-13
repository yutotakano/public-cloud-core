#include "info.h"
#include "utils.h"

InfoApp::InfoApp()
{
  logger = quill::get_logger();
  executor = Executor();
}

std::unique_ptr<argparse::ArgumentParser> InfoApp::info_arg_parser()
{
  auto parser = std::make_unique<argparse::ArgumentParser>("info");
  parser->add_description("Get information about the current deployment.");
  return parser;
}

void InfoApp::info_command_handler(argparse::ArgumentParser &parser)
{
  LOG_TRACE_L3(logger, "Getting information about the current deployment.");

  auto info = get_info();
  if (!info.has_value())
  {
    LOG_ERROR(logger, "Could not get info: no active deployment found.");
    return;
  }

  LOG_INFO(logger, "CoreKube deployment found.");
  LOG_INFO(logger, "IP (within VPC) of CK frontend: {}", info->frontend_ip);
  LOG_INFO(logger, "Grafana: http://{}:3000", info->corekube_dns_name);
  LOG_INFO(logger, "Nervion: http://{}:8080", info->nervion_dns_name);
}

std::optional<deployment_info_s> InfoApp::get_info()
{
  LOG_DEBUG(logger, "Looking for active deployment.");

  // Check if there is an active deployment
  LOG_TRACE_L3(logger, "Checking existing contexts");
  auto contexts_str =
    executor.run({"kubectl", "config", "get-contexts", "--output=name"}, false)
      .future.get();
  auto contexts = Utils::split(contexts_str, '\n');
  if (contexts.size() == 1 && contexts[0].empty())
  {
    LOG_TRACE_L3(logger, "No active deployment found.");
    return std::nullopt;
  }

  // Double-check that there is a CoreKube and Nervion deployment
  std::string corekube_name;
  std::string nervion_name;
  for (auto &context : contexts)
  {
    if (context.find("corekube") != std::string::npos)
    {
      LOG_TRACE_L3(logger, "CoreKube context found: {}", context);
      corekube_name = context;
    }
    if (context.find("nervion") != std::string::npos)
    {
      LOG_TRACE_L3(logger, "Nervion context found: {}", context);
      nervion_name = context;
    }
  }
  if (corekube_name.empty() || nervion_name.empty())
  {
    LOG_TRACE_L3(logger, "No active deployment found.");
    return std::nullopt;
  }

  // Switch to the CoreKube context to get information about frontend
  LOG_TRACE_L3(logger, "Switching to CoreKube context");
  executor.run({"kubectl", "config", "use-context", corekube_name}, false)
    .future.get();

  // Get the VPC-internal IP of the frontend node
  LOG_TRACE_L3(logger, "Getting frontend IP");
  auto frontend_ip =
    executor
      .run(
        {"kubectl",
         "get",
         "nodes",
         "-o",
         "jsonpath={.items[0].status.addresses[?(@.type==\"InternalIP\")]."
         "address}",
         "-l",
         "alpha.eksctl.io/nodegroup-name=ng-corekube"},
        false
      )
      .future.get();

  // Get the public DNS of the Grafana service.
  LOG_TRACE_L3(logger, "Getting Grafana public DNS");
  auto ck_grafana_public_dns =
    executor
      .run(
        {"kubectl",
         "get",
         "services/corekube-grafana",
         "-n=grafana",
         "-o=jsonpath={.status.loadBalancer.ingress[0].hostname}"},
        false
      )
      .future.get();

  // Switch to the Nervion context to get information about Nervion controller
  LOG_TRACE_L3(logger, "Switching to Nervion context");
  executor.run({"kubectl", "config", "use-context", nervion_name}, false)
    .future.get();

  // Get the public DNS of the Nervion controller service.
  auto nervion_controller_public_dns =
    executor
      .run(
        {
          "kubectl",
          "get",
          "services/ran-emulator",
          "-o=jsonpath={.status.loadBalancer.ingress[0].hostname}",
        },
        false
      )
      .future.get();

  return deployment_info_s{
    .corekube_dns_name = ck_grafana_public_dns,
    .nervion_dns_name = nervion_controller_public_dns,
    .frontend_ip = frontend_ip,
  };
}
