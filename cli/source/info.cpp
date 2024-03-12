#include "info.h"
#include "utils.h"

InfoApp::InfoApp()
{
  logger = quill::get_logger();
  executor = Executor();
  ck_app = CKApp();
  nv_app = NVApp();
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

bool InfoApp::check_contexts_exist()
{
  LOG_TRACE_L3(logger, "Checking existing contexts");

  auto contexts_str =
    executor
      .run(
        {"kubectl",
         "config",
         "get-contexts",
         "--output=name",
         "--kubeconfig",
         "config"},
        false,
        false,
        false
      )
      .future.get();
  auto contexts = Utils::split(contexts_str, '\n');
  if (contexts.size() == 1 && contexts[0].empty())
  {
    LOG_TRACE_L3(logger, "No contexts found.");
    return false;
  }

  std::string corekube_context;
  std::string nervion_context;
  for (auto &context : contexts)
  {
    if (context.find("corekube") != std::string::npos)
    {
      corekube_context = context;
      LOG_TRACE_L3(logger, "CoreKube context found: {}", context);
    }
    if (context.find("nervion") != std::string::npos)
    {
      nervion_context = context;
      LOG_TRACE_L3(logger, "Nervion context found: {}", context);
    }
  }
  if (corekube_context.empty() || nervion_context.empty())
  {
    LOG_TRACE_L3(logger, "No active deployment found.");
    return false;
  }

  return true;
}

std::optional<deployment_info_s> InfoApp::get_info()
{
  LOG_DEBUG(logger, "Looking for active deployment.");

  // Check if there is an active deployment
  if (!check_contexts_exist())
  {
    return std::nullopt;
  }

  // Get the VPC-internal IP of the frontend node
  LOG_TRACE_L3(logger, "Getting frontend IP");
  auto frontend_ip =
    ck_app
      .run_kubectl(
        {"get",
         "nodes",
         "-o",
         "jsonpath={.items[0].status.addresses[?(@.type==\"InternalIP\")]."
         "address}",
         "-l",
         "eks.amazonaws.com/nodegroup"},
        false
      )
      .future.get();

  // Get the public DNS of the Grafana service.
  LOG_TRACE_L3(logger, "Getting Grafana public service name");
  auto ck_grafana_public_dns =
    ck_app
      .run_kubectl(
        {"get",
         "services/corekube-grafana",
         "-o=jsonpath={.status.loadBalancer.ingress[0].hostname}"},
        false
      )
      .future.get();

  // Get the public DNS of the Nervion controller service.
  auto nervion_controller_public_dns =
    nv_app
      .run_kubectl(
        {
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
