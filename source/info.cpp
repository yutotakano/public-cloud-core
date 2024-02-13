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

std::optional<context_info_s> InfoApp::get_contexts()
{
  LOG_TRACE_L3(logger, "Checking existing contexts");

  auto contexts_str =
    executor.run({"kubectl", "config", "get-contexts", "--output=name"}, false)
      .future.get();
  auto contexts = Utils::split(contexts_str, '\n');
  if (contexts.size() == 1 && contexts[0].empty())
  {
    LOG_TRACE_L3(logger, "No contexts found.");
    return std::nullopt;
  }

  std::string corekube_context;
  std::string nervion_context;
  for (auto &context : contexts)
  {
    if (context.find("corekube") != std::string::npos)
    {
      LOG_TRACE_L3(logger, "CoreKube context found: {}", context);
      corekube_context = context;
    }
    if (context.find("nervion") != std::string::npos)
    {
      LOG_TRACE_L3(logger, "Nervion context found: {}", context);
      nervion_context = context;
    }
  }
  if (corekube_context.empty() || nervion_context.empty())
  {
    LOG_TRACE_L3(logger, "No active deployment found.");
    return std::nullopt;
  }

  std::string current_context =
    executor.run({"kubectl", "config", "current-context"}, false).future.get();

  return context_info_s{
    .current_context = current_context,
    .corekube_context = corekube_context,
    .nervion_context = nervion_context,
  };
}

void InfoApp::switch_context(std::string context)
{
  LOG_DEBUG(logger, "Switching to context: {}", context);
  executor.run({"kubectl", "config", "use-context", context}, false)
    .future.get();
}

std::optional<deployment_info_s> InfoApp::get_info()
{
  LOG_DEBUG(logger, "Looking for active deployment.");

  // Check if there is an active deployment
  auto contexts = get_contexts();
  if (!contexts.has_value())
  {
    LOG_TRACE_L3(logger, "No active deployment found.");
    return std::nullopt;
  }

  // Switch to the CoreKube context to get information about frontend
  switch_context(contexts->corekube_context);

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
  switch_context(contexts->nervion_context);

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

  // Switch back to the original context
  switch_context(contexts->current_context);

  return deployment_info_s{
    .corekube_dns_name = ck_grafana_public_dns,
    .nervion_dns_name = nervion_controller_public_dns,
    .frontend_ip = frontend_ip,
  };
}
